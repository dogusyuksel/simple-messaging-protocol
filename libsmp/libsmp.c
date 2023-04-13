#include "smp_common.h"

struct callback_list_entry {
	int (*callback)(void *, unsigned int);
	char *title;
	TAILQ_ENTRY(callback_list_entry) entries;
};

TAILQ_HEAD(callback_list, callback_list_entry);

struct smp_identifier
{
	bool server_started;
	unsigned int port;
	struct callback_list callback_list;
};

typedef struct smp_identifier _smp_identifier;

static _smp_identifier identifier;


static int smp_send_packet(struct _packet *packet, int fd)
{
	char buffer[BUFFER_SIZE] = {0};
	json_object *message = NULL;

	if (!packet || !packet->title || fd < 0) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	message = json_object_new_object();
	if (message == NULL) {
		return NOK;
	}
	json_object_object_add(message, JSON_STR_TYPE,
		json_object_new_int((unsigned int)packet->packet_type));
	json_object_object_add(message, JSON_STR_PORT,
		json_object_new_int(packet->port));
	if (packet->title) {
		json_object_object_add(message, JSON_STR_TITLE,
			json_object_new_string(packet->title));
	} else {
		json_object_object_add(message, JSON_STR_TITLE,
			json_object_new_string(JSON_STR_EMPTY));
	}
	if (packet->payload) {
		json_object_object_add(message, JSON_STR_PAYLOAD,
			json_object_new_string(packet->payload));
	} else {
		json_object_object_add(message, JSON_STR_PAYLOAD,
			json_object_new_string(JSON_STR_EMPTY));
	}

	strcpy(buffer, json_object_to_json_string(message));
	debugf("%s\n", buffer);

	errno = 0;
	if (packet->title &&
		send(fd, buffer, strlen(buffer), MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	json_object_put(message);

	return OK;
}

static struct callback_list_entry *smp_find_callback(char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!title) {
		errorf("args cannot be NULL\n");
		return NULL;
	}

	TAILQ_FOREACH(entry, &(identifier.callback_list), entries) {
		if (entry->title && strcmp(entry->title, title) == 0) {
			return entry;
		}
	}

	return NULL;
}

static int smp_delete_all_callback_list(void)
{
	struct callback_list_entry *entry1 = NULL, *entry2 = NULL;

	entry1 = TAILQ_FIRST(&(identifier.callback_list));
	while (entry1 != NULL) {
		entry2 = TAILQ_NEXT(entry1, entries);
		FREE(entry1->title);
		FREE(entry1);
		entry1 = entry2;
	}

	TAILQ_INIT(&(identifier.callback_list)); 

	return OK;
}

static int smp_read_data(int sockfd, bool *destroy)
{
	int ret = OK;
	struct _packet packet;
	char buffer[BUFFER_SIZE] = {0};
	struct callback_list_entry *entry = NULL;
	struct json_object *object = NULL;
	struct json_object *type = NULL;
	struct json_object *port = NULL;
	struct json_object *title = NULL;
	struct json_object *payload = NULL;

	if (sockfd < 0 || !destroy) {
		errorf("args cannot be NULL\n");
		goto fail;
	}

	memset(&packet, 0, sizeof(struct _packet));
	memset(buffer, 0, sizeof(buffer));

	errno = 0;
	if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	object = json_tokener_parse(buffer);
	if (object == NULL) {
		errorf("json_tokener_parse failed\n");
		goto fail;
	}
	type = json_object_object_get(object, JSON_STR_TYPE);
	if (type == NULL || !json_object_is_type(type, json_type_int)) {
		errorf("cannot get %s field\n", JSON_STR_TYPE);
		goto fail;
	}
	packet.packet_type = (enum _packet_type)json_object_get_int(type);

	port = json_object_object_get(object, JSON_STR_PORT);
	if (port == NULL || !json_object_is_type(port, json_type_int)) {
		errorf("cannot get %s field\n", JSON_STR_PORT);
		goto fail;
	}
	packet.port = json_object_get_int(port);

	title = json_object_object_get(object, JSON_STR_TITLE);
	if (title == NULL || !json_object_is_type(title, json_type_string)) {
		errorf("cannot get %s field\n", JSON_STR_TITLE);
		goto fail;
	}
	packet.title = strdup(json_object_get_string(title));
	if (!packet.title) {
		errorf("strdup failed\n");
		goto fail;
	}

	payload = json_object_object_get(object, JSON_STR_PAYLOAD);
	if (payload == NULL || !json_object_is_type(payload, json_type_string)) {
		errorf("cannot get %s field\n", JSON_STR_PAYLOAD);
		goto fail;
	}
	packet.payload = strdup(json_object_get_string(payload));
	if (!packet.payload) {
		errorf("strdup failed\n");
		goto fail;
	}

	if (packet.packet_type == SENDATA && packet.payload && strlen(packet.payload)) {
		if ((entry = smp_find_callback(packet.title)) == NULL) {
			errorf("cannot find callback\n");
			goto fail;
		}
		entry->callback(packet.payload, strlen(packet.payload));
	} else if (packet.packet_type == DESTROY) {
		debugf("thread wants to be destroyed\n");
		*destroy = true;
	}

	goto out;

fail:
	ret = NOK;

out:
	FREE(packet.title);
	FREE(packet.payload);
	json_object_put(object);

	return ret;
}

static void *smp_create_server(void *arg)
{
	int listen_fd, conn_fd, max_fd = 1, ret_val, i, enable = 1;
	unsigned int port = 0;
	bool destroy_reuested = false;
	struct sockaddr_storage client_addr, server_addr;
	struct timeval tv;
	char c_ip_addr[INET6_ADDRSTRLEN] = {0};
	fd_set backup_set, client_set;

	if (!(unsigned int *)arg) {
		errorf("arg is null\n");
		goto out;
	}

	port = *((unsigned int *)arg);
	memset(c_ip_addr, 0, sizeof(c_ip_addr));
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	if (smp_common_fill_wildcard_sockstorage(port, &server_addr) != 0) {
		errorf("smp_common_fill_wildcard_sockstorage() failed\n");
		goto out;
	}

	if ((listen_fd =
		smp_common_socket_open_use_sockaddr((struct sockaddr *)&server_addr)) == -1) {
		errorf("smp_common_socket_open_use_sockaddr() failed\n");
		goto out;
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		errorf("setsockopt reuseport fail\n");
		goto out;
	}

	if (smp_common_bind_socket(listen_fd, (struct sockaddr *)&server_addr) == -1) {
		errorf("smp_common_bind_socket() failed\n");
		goto out;
	}

	if (smp_common_socket_listen(listen_fd) == -1) {
		errorf("smp_common_socket_listen() failed\n");
		goto out;
	}

	FD_ZERO(&backup_set);
	max_fd = listen_fd;
	FD_SET(listen_fd, &backup_set);
	tv.tv_usec = 0;

	while (!destroy_reuested) {
		tv.tv_sec = RECEIVE_TIMEOUT;
		memcpy(&client_set, &backup_set, sizeof(backup_set));

		ret_val = select(max_fd + 1, &client_set, NULL, NULL, &tv);

		if (ret_val < 0) {
			errorf("select error\n");
			continue;
		} else if (ret_val == 0) {
			for (i = 0; i <= max_fd; i++) {
				if (FD_ISSET(i, &backup_set) && i != listen_fd) {
						close(i);
						FD_CLR(i, &backup_set);
				}
			}
			max_fd = listen_fd;
			continue;
		}

		if (FD_ISSET(listen_fd, &client_set)) {
			conn_fd = smp_common_socket_accept(listen_fd, &client_addr);
			if (conn_fd < 0) {
				if (errno == EINTR) {
					debugf("CRS interrupted, try to accept again\n");
					continue;
				}
				errorf("accept error\n");
				goto out;
			}

			FD_SET(conn_fd, &backup_set);
			if (conn_fd > max_fd) {
				max_fd = conn_fd;
			}
			continue;
		}

		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i, &client_set) && i != listen_fd) {
				if (smp_read_data(i, &destroy_reuested) == NOK) {
					errorf("smp_read_data() failed\n");
					goto out;
				}
				close(i);
				FD_CLR(i, &backup_set);
			}
		}
	}

out:
	for (i = 0; i <= max_fd; i++) {
		close(i);
		FD_CLR(i, &backup_set);
	}

	debugf("thread destroyed\n");
	pthread_exit(NULL);

	return NULL;
}

static void smp_create_server_thread(void)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;

	if (pthread_attr_init(&thread_attr) != 0) {
		errorf("pthread_attr_init failure.\n");
		goto fail;
	}

	if (pthread_attr_setdetachstate(&thread_attr,
		PTHREAD_CREATE_DETACHED) != 0 ) {
		errorf("pthread_attr_setdetachstate failure.\n");
		goto fail;
	}

	errno = 0;
	if (pthread_create(&thread_id, &thread_attr,
		smp_create_server, (void *)(&(identifier.port))) != 0) {
		errorf("pthread_create failure, errno: %d\n", errno);
		goto fail;
	}

	identifier.server_started = true;

	goto out;

fail:
	errorf("smp_create_server_thread() failed\n");

out:
	return;
}

static int smp_delete_callback_from_callback_list(char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	if (!(&(identifier.callback_list)) ||
		TAILQ_EMPTY(&(identifier.callback_list))) {
		return OK;
	}

	TAILQ_FOREACH(entry, &(identifier.callback_list), entries) {
		if (entry && entry->title && strcmp(title, entry->title) == 0) {
			TAILQ_REMOVE(&(identifier.callback_list), entry, entries);
			FREE(entry->title);
			FREE(entry);
			break;
		}
	}

	if ((&(identifier.callback_list)) &&
		TAILQ_EMPTY(&(identifier.callback_list))) {
		TAILQ_INIT(&(identifier.callback_list));
	}

	return OK;
}

static int smp_find_callback_in_callback_list
	(int (*callback)(void *, unsigned int), char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!callback || !title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	TAILQ_FOREACH(entry, &(identifier.callback_list), entries) {
		if (entry->title && strcmp(title, entry->title) == 0) {
			entry->callback = callback; //edit callback
			return OK;
		}
	}

	return NOK;
}

static int smp_add_callback_to_callback_list
	(int (*callback)(void *, unsigned int), char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!callback || !title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	entry = (struct callback_list_entry *)
		calloc(1, sizeof(struct callback_list_entry));
	if (!entry) {
		errorf("calloc failed\n");
		return NOK;
	}

	entry->callback = callback;

	entry->title = (char *)calloc(1, strlen(title) + 1);
	if (!entry->title) {
		errorf("calloc failed\n");
		FREE(entry->callback);
		FREE(entry);
		return NOK;
	}
	strcpy(entry->title, title);

	TAILQ_INSERT_HEAD(&(identifier.callback_list), entry, entries);

	return OK;
}

static int smp_send(char *title, int (*callback)(void *, unsigned int), enum _packet_type packet_type,
	void *data, unsigned int port, unsigned long timeout)
{
	int ret = NOK;
	int fd  = - 1;
	unsigned int len = 0;
	unsigned int local_svr_port = 0;
	unsigned long timeout_cnt = 0;
	struct sockaddr_storage address;
	struct _packet packet;

	if (!title) {
		errorf("title cannot be NULL\n");
		return NOK;
	}

	if (packet_type != REGISTER && !identifier.server_started) {
		return NOK;
	}

	memset((void *)&address, 0, sizeof(struct sockaddr_in6));
	memset(&packet, 0, sizeof(struct _packet));

	local_svr_port = identifier.port;

	if (smp_common_buf_to_sockstorage(IPV6_LOOPBACK_ADDR, port, &address) == NOK) {
		errorf("smp_common_buf_to_sockstorage() failed\n");
		return NOK;
	}

	while (timeout_cnt <= timeout) {
		if (fd) {
			close(fd);
		}
		sleep(1);
		timeout_cnt++;

		fd = smp_common_socket_open_use_buf();
		if (fd == -1) {
			errorf("socket() failed with %d: %s\n", errno, strerror(errno));
			continue;
		}

		if (smp_common_connect_socket(fd, (struct sockaddr*)&address) < 0) {
			errorf("connect() failed with %d: %s\n", errno, strerror(errno));
			continue;
		}

		ret = OK;
		break;
	}

	if (ret != OK || fd < 0) {
		debugf("retry failed\n");
		goto fail;
	}

	packet.title = strdup(title);
	if (!packet.title) {
		errorf("strdup failed\n");
		goto fail;
	}
	packet.packet_type = (unsigned char)packet_type;
	packet.payload = NULL;

	if (data && (len = strlen((char *)data))) {
		packet.payload = (void *)calloc(1, len + 1);
		if (!packet.payload) {
			errorf("calloc failed\n");
			goto fail;
		}
		strcpy(packet.payload, data);
	}

	packet.port = identifier.port;

	if (smp_send_packet(&packet, fd) == NOK) {
		errorf("smp_send_packet() failed with %d: %s\n",
			errno, strerror(errno));
		goto fail;
	}

	if (!identifier.server_started) {
		if (recv(fd, &local_svr_port, sizeof(unsigned int), 0) == -1) {
			errorf("recv() failed with %d: %s\n", errno, strerror(errno));
			goto fail;
		}

		if (local_svr_port < STARTING_PORT ||
			local_svr_port > STARTING_PORT + BACKLOG) {
			debugf("need to register first\n");
		} else {
			debugf("port %d initialized for this app\n", local_svr_port);
			identifier.port = local_svr_port;
			TAILQ_INIT(&(identifier.callback_list));
			smp_create_server_thread();
		}
	}

	if (packet_type == REGISTER) {
		if (!callback) {
			errorf("callback cannot be NULL while registering\n");
			goto fail;
		}
		if (smp_find_callback_in_callback_list(callback, title) == NOK) {
			if (smp_add_callback_to_callback_list(callback, title) == NOK) {
				errorf("smp_add_callback_to_callback_list() failed\n");
				goto fail;
			}
		}
	} else if (packet_type == UNREGISTER) {
		if (smp_delete_callback_from_callback_list(title) == NOK) {
			errorf("smp_delete_callback_from_callback_list() failed\n");
			goto fail;
		}
	} else if (packet_type == UNREGISTER_ALL) {
		if (smp_delete_all_callback_list() == NOK) {
			errorf("smp_delete_all_callback_list() failed\n");
			goto fail;
		}
	}

	goto out;

fail:
	ret = NOK;

out:
	if (fd) {
		close(fd);
	}
	FREE(packet.title);
	FREE(packet.payload);

	return ret;
}

int smp_register(char *title, int (*callback)(void *, unsigned int), ...)
{
	va_list args;
	const char *fmt = "%d";
	char buffer[BUFFER_SIZE];
	char *ptr = NULL;
	unsigned long timeout = 0;

	if (!title || !callback) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	va_end(args);

	timeout = strtoul(buffer, &ptr, 10);

	return smp_send(title, callback, REGISTER, NULL, PORT, timeout);
}

static int smp_unregister_all(void)
{
	char unreg_buf[256] = {0};

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", identifier.port);

	return smp_send(DUMMY_STRING, NULL, UNREGISTER_ALL, unreg_buf, PORT, 0);
}

int smp_destroy(void)
{
	if (smp_unregister_all() == NOK) {
		return NOK;
	}
	if (smp_send(DUMMY_STRING, NULL, DESTROY, NULL, identifier.port, 0) == NOK) {
		return NOK;
	}

	sleep(2);	//this is here for observing data release for valgrind

	return OK;
}

int smp_unregister(char *title)
{
	char unreg_buf[256] = {0};

	if (!title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", identifier.port);
	return smp_send(title, NULL, UNREGISTER, unreg_buf, PORT, 0);
}

int smp_send_data(char *title, void *data, ...)
{
	va_list args;
	const char *fmt = "%d";
	char buffer[BUFFER_SIZE];
	char *ptr = NULL;
	unsigned long timeout = 0;

	if (!title || !data) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	va_end(args);

	timeout = strtoul(buffer, &ptr, 10);

	return smp_send(title, NULL, SENDATA, data, PORT, timeout);
}

int smp_send_bradcast_data(void *data, ...)
{
	va_list args;
	const char *fmt = "%d";
	char buffer[BUFFER_SIZE];
	char *ptr = NULL;
	unsigned long timeout = 0;

	if (!data) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	va_end(args);

	timeout = strtoul(buffer, &ptr, 10);

	return smp_send(BROADCAST_UNIQUE_TITLE, NULL, SENDATA, data, PORT, timeout);
}

int smp_broadcast_register(int (*callback)(void *, unsigned int), ...)
{
	va_list args;
	const char *fmt = "%d";
	char buffer[BUFFER_SIZE];
	char *ptr = NULL;
	unsigned long timeout = 0;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	va_end(args);

	timeout = strtoul(buffer, &ptr, 10);

	return smp_register(BROADCAST_UNIQUE_TITLE, callback, timeout);
}

int smp_broadcast_unregister(void)
{
	return smp_unregister(BROADCAST_UNIQUE_TITLE);
}
