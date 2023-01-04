#include "sipc_common.h"

struct callback_list_entry {
	int (*callback)(void *, unsigned int);
	char *title;
	TAILQ_ENTRY(callback_list_entry) entries;
};

TAILQ_HEAD(callback_list, callback_list_entry);

struct sipc_identifier
{
	bool server_started;
	unsigned int port;
	struct callback_list callback_list;
};

typedef struct sipc_identifier _sipc_identifier;

static _sipc_identifier identifier;

static int sipc_send_packet(struct _packet *packet, int fd)
{
	if (!packet || !packet->title || fd <= 0) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->packet_type, sizeof(packet->packet_type), MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->title_size, sizeof(packet->title_size), MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (packet->title_size && send(fd, packet->title, packet->title_size, MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->payload_size, sizeof(packet->payload_size), MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (packet->payload_size && packet->payload && send(fd, packet->payload, packet->payload_size, MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &(identifier.port), sizeof(identifier.port), MSG_NOSIGNAL) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	return OK;
}

static struct callback_list_entry * find_callback(char *title)
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

static int delete_all_callback_list(void)
{
	struct callback_list_entry *entry1 = NULL;
	struct callback_list_entry *entry2 = NULL;

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

static int sipc_read_data(int sockfd, bool *destroy)
{
	int ret = OK;
	struct _packet packet;
	struct callback_list_entry *entry = NULL;

	if (sockfd <= 0 || !destroy) {
		errorf("args cannot be NULL\n");
		goto fail;
	}

	memset(&packet, 0, sizeof(struct _packet));

	errno = 0;
	if (recv(sockfd, &packet.packet_type, sizeof(packet.packet_type), 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	errno = 0;
	if (recv(sockfd, &packet.title_size, sizeof(packet.title_size), 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	if (!packet.title_size) {
		errorf("packet title size should be greater than zero\n");
		goto out;
	}
	packet.title = (char *)calloc(packet.title_size, sizeof(char));
	if (!packet.title) {
		errorf("calloc failed\n");
		goto fail;
	}
	errno = 0;
	if (recv(sockfd, packet.title, packet.title_size, 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	errno = 0;
	if (recv(sockfd, &packet.payload_size, sizeof(packet.payload_size), 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	if (!packet.payload_size) {
		goto proceed;
	}

	packet.payload = (char *)calloc(packet.payload_size + 1, sizeof(char));
	if (!packet.payload) {
		errorf("calloc failed\n");
		goto fail;
	}
	errno = 0;
	if (recv(sockfd, packet.payload, packet.payload_size, 0) < 0) {
		errorf("recv error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

proceed:
	if (packet.packet_type == SENDATA && packet.payload && packet.payload_size) {
		if ((entry = find_callback(packet.title)) == NULL) {
			errorf("cannot find callback\n");
			goto fail;
		}
		entry->callback(packet.payload, packet.payload_size);
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

	return ret;
}

static void *sipc_create_server(void *arg)
{
	unsigned int port = 0;
	int enable = 1;
	int listen_fd, conn_fd, max_fd, ret_val, i;
	bool destroy_reuested = false;
	struct sockaddr_storage client_addr, server_addr;
	char c_ip_addr[INET6_ADDRSTRLEN] = {0};
	fd_set backup_set, client_set;
	struct timeval tv;

	if (!(unsigned int *)arg) {
		errorf("arg is null\n");
		goto out;
	}

	port = *((unsigned int *)arg);
	memset(c_ip_addr, 0, sizeof(c_ip_addr));
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	if (sipc_fill_wildcard_sockstorage(port, AF_UNSPEC, &server_addr) != 0) {
		errorf("sipc_fill_wildcard_sockstorage() failed\n");
		goto out;
	}

	if ((listen_fd = sipc_socket_open_use_sockaddr((struct sockaddr *)&server_addr, SOCK_STREAM, 0)) == -1) {
		errorf("sipc_socket_open_use_sockaddr() failed\n");
		goto out;
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR , &enable, sizeof(int)) < 0) {
		errorf("setsockopt reuseport fail\n");
		goto out;
	}

	if (sipc_bind_socket(listen_fd, (struct sockaddr *)&server_addr) == -1) {
		errorf("sipc_bind_socket() failed\n");
		goto out;
	}

	if (sipc_socket_listen(listen_fd, BACKLOG) == -1) {
		errorf("sipc_socket_listen() failed\n");
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
			conn_fd = sipc_socket_accept(listen_fd, &client_addr);
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
				if (sipc_read_data(i, &destroy_reuested) == NOK) {
					errorf("sipc_read_data() failed\n");
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

static void create_server_thread(void)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;

	if (pthread_attr_init(&thread_attr) != 0) {
		errorf("pthread_attr_init failure.\n");
		goto fail;
	}

	if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0 ) {
		errorf("pthread_attr_setdetachstate failure.\n");
		goto fail;
	}

	errno = 0;
	if (pthread_create(&thread_id, &thread_attr, sipc_create_server, (void *)(&(identifier.port))) != 0) {
		errorf("pthread_create failure, errno: %d\n", errno);
		goto fail;
	}

	identifier.server_started = true;

	goto out;

fail:
	errorf("create_server_thread() failed\n");

out:
	return;
}

static int delete_callback_from_callback_list(char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	if (!(&(identifier.callback_list)) || TAILQ_EMPTY(&(identifier.callback_list))) {
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

	if ((&(identifier.callback_list)) && TAILQ_EMPTY(&(identifier.callback_list))) {
		TAILQ_INIT(&(identifier.callback_list));
	}

	return OK;
}

static int find_callback_in_callback_list(int (*callback)(void *, unsigned int), char *title)
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

static int add_callback_to_callback_list(int (*callback)(void *, unsigned int), char *title)
{
	struct callback_list_entry *entry = NULL;

	if (!callback || !title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	entry = (struct callback_list_entry *)calloc(1, sizeof(struct callback_list_entry));
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

static int sipc_send(char *title, int (*callback)(void *, unsigned int), enum _packet_type packet_type,
	void *data, unsigned int len, unsigned int _port)
{
	int ret = OK;
	int fd;
	unsigned int local_svr_port = 0;
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

	if (sipc_buf_to_sockstorage(IPV6_LOOPBACK_ADDR, _port, &address) == NOK) {
		errorf("sipc_buf_to_sockstorage() failed\n");
		return NOK;
	}

	fd = sipc_socket_open_use_buf(IPV6_LOOPBACK_ADDR, SOCK_STREAM,0);
	if (fd == -1) {
		errorf("socket() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (sipc_connect_socket(fd, (struct sockaddr*)&address) == NOK) {
		errorf("connect() failed with %d: %s\n", errno, strerror(errno));
		goto fail;
	}

	packet.title = strdup(title);
	if (!packet.title) {
		errorf("strdup failed\n");
		goto fail;
	}
	packet.title_size = strlen(title) + 1;
	packet.packet_type = (unsigned char)packet_type;
	packet.payload_size = 0;
	packet.payload = NULL;

	if (data && len) {
		packet.payload = (void *)calloc(1, len + 1);
		if (!packet.payload) {
			errorf("calloc failed\n");
			goto fail;
		}
		strncpy(packet.payload, data, len);
		packet.payload_size = len + 1;
	}

	if (sipc_send_packet(&packet, fd) == NOK) {
		errorf("sipc_send_packet() failed with %d: %s\n", errno, strerror(errno));
		goto fail;
	}

	if (!identifier.server_started) {
		if (recv(fd, &local_svr_port, sizeof(unsigned int), 0) == -1) {
			errorf("recv() failed with %d: %s\n", errno, strerror(errno));
			goto fail;
		}

		if (local_svr_port < STARTING_PORT || local_svr_port > STARTING_PORT + BACKLOG) {
			debugf("need to register first\n");
		} else {
			debugf("port %d initialized for this app\n", local_svr_port);
			identifier.port = local_svr_port;
			TAILQ_INIT(&(identifier.callback_list));
			create_server_thread();
		}
	}

	if (packet_type == REGISTER) {
		if (!callback) {
			errorf("callback cannot be NULL while registering\n");
			goto fail;
		}
		if (find_callback_in_callback_list(callback, title) == NOK) {
			if (add_callback_to_callback_list(callback, title) == NOK) {
				errorf("add_callback_to_callback_list() failed\n");
				goto fail;
			}
		}
	} else if (packet_type == UNREGISTER) {
		if (delete_callback_from_callback_list(title) == NOK) {
			errorf("delete_callback_from_callback_list() failed\n");
			goto fail;
		}
	} else if (packet_type == UNREGISTER_ALL) {
		if (delete_all_callback_list() == NOK) {
			errorf("delete_all_callback_list() failed\n");
			goto fail;
		}
	}

	goto out;

fail:
	ret = NOK;

out:
	close(fd);
	FREE(packet.title);
	FREE(packet.payload);

	return ret;
}

int sipc_register(char *title, int (*callback)(void *, unsigned int))
{
	if (!title || !callback) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, callback, REGISTER, NULL, 0, PORT);
}

static int sipc_unregister_all(void)
{
	char unreg_buf[256] = {0};

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", identifier.port);

	return sipc_send(DUMMY_STRING, NULL, UNREGISTER_ALL, unreg_buf, strlen(unreg_buf), PORT);
}

int sipc_destroy(void)
{
	if (sipc_unregister_all() == NOK) {
		return NOK;
	}
	if (sipc_send(DUMMY_STRING, NULL, DESTROY, NULL, 0, identifier.port) == NOK) {
		return NOK;
	}

	sleep(2);	//this is here for observing data release for valgrind

	return OK;
}

int sipc_unregister(char *title)
{
	char unreg_buf[256] = {0};

	if (!title) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", identifier.port);
	return sipc_send(title, NULL, UNREGISTER, unreg_buf, strlen(unreg_buf), PORT);
}

int sipc_send_data(char *title, void *data, unsigned int len)
{
	if (!title || !data || !len) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, NULL, SENDATA, data, len, PORT);
}

int sipc_bradcast_data(void *data, unsigned int len)
{
	if (!data || !len) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(BROADCAST_UNIQUE_TITLE, NULL, SENDATA, data, len, PORT);
}

int sipc_broadcast_register(int (*callback)(void *, unsigned int))
{
	return sipc_register(BROADCAST_UNIQUE_TITLE, callback);
}

int sipc_broadcast_unregister(void)
{
	return sipc_unregister(BROADCAST_UNIQUE_TITLE);
}
