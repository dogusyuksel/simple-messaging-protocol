#include "sipc_common.h"

struct _thread_data
{
	int (*callback)(void *, unsigned int);
	unsigned int port;
};

static int sipc_send_packet(struct _packet *packet, int fd, _sipc_identifier *identifier)
{
	unsigned int local_port = *identifier;

	if (!packet || !packet->title || fd < 0) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->packet_type, sizeof(packet->packet_type), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->title_size, sizeof(packet->title_size), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (packet->title_size && send(fd, packet->title, packet->title_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &packet->payload_size, sizeof(packet->payload_size), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (packet->payload_size && packet->payload && send(fd, packet->payload, packet->payload_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	errno = 0;
	if (send(fd, &local_port, sizeof(local_port), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	return OK;
}

static int sipc_read_data(int sockfd,  int (*callback)(void *, unsigned int), bool *destoy)
{
	int ret = OK;
	struct _packet packet;

	memset(&packet, 0, sizeof(struct _packet));

	errno = 0;
	if (read(sockfd, &packet.packet_type, sizeof(packet.packet_type)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	errno = 0;
	if (read(sockfd, &packet.title_size, sizeof(packet.title_size)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
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
	if (read(sockfd, packet.title, packet.title_size) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	errno = 0;
	if (read(sockfd, &packet.payload_size, sizeof(packet.payload_size)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
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
	if (read(sockfd, packet.payload, packet.payload_size) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

proceed:
	if (callback && packet.packet_type == SENDATA && packet.payload_size) {
		callback(packet.payload, packet.payload_size);
	} else if (packet.packet_type == DESTROY) {
		debugf("thread wants to be destroyed\n");
		*destoy = true;
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
	struct _thread_data *thdata = arg;
	int enable = 1;
	int listen_fd, conn_fd, max_fd, ret_val, i;
	bool destroy_reuested = false;
	struct sockaddr_storage client_addr, server_addr;
	char c_ip_addr[INET6_ADDRSTRLEN] = {0};
	fd_set backup_set, client_set;
	struct timeval tv;

	if (!thdata) {
		errorf("arg is null\n");
		goto out;
	}

	memset(c_ip_addr, 0, sizeof(c_ip_addr));
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	if (sipc_fill_wildcard_sockstorage(thdata->port, AF_UNSPEC, &server_addr) != 0) {
		errorf("sipc_fill_wildcard_sockstorage() failed\n");
		goto out;
	}

	if ((listen_fd = sipc_socket_open_use_sockaddr((struct sockaddr *)&server_addr, SOCK_STREAM, 0)) == -1) {
		errorf("sipc_socket_open_use_sockaddr() failed\n");
		goto out;
	}

	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR , &enable, sizeof(int)) < 0) {
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
				if (sipc_read_data(i,  thdata->callback, &destroy_reuested) == NOK) {
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

	FREE(thdata);
	debugf("thread destroyed\n");
	pthread_exit(NULL);

	return NULL;
}

static void create_server_thread(int (*callback)(void *, unsigned int), unsigned int port)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	struct _thread_data *thdata = NULL;

	if (!callback || !port) {
		errorf("parameters are null\n");
		goto fail;
	}

	if (pthread_attr_init(&thread_attr) != 0) {
		errorf("pthread_attr_init failure.\n");
		goto fail;
	}

	if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0 ) {
		errorf("pthread_attr_setdetachstate failure.\n");
		goto fail;
	}

	thdata = (void *)calloc(1, sizeof(struct _thread_data));
	if (!thdata) {
		errorf("calloc failed\n");
		goto fail;
	}

	thdata->callback = callback;
	thdata->port = port;

	errno = 0;
	if (pthread_create(&thread_id, &thread_attr, sipc_create_server, (void *)thdata) != 0) {
		errorf("pthread_create failure, errno: %d\n", errno);
		goto fail;
	}

	goto out;

fail:
	errorf("create_server_thread() failed\n");

out:
	return;
}

static int sipc_send(char *title_arg, int (*callback)(void *, unsigned int), enum _packet_type packet_type,
	void *data, unsigned int len, unsigned int _port, _sipc_identifier *identifier)
{
	int ret = OK;
	int fd;
	unsigned int local_svr_port = *identifier;
	struct sockaddr_storage address;
	struct _packet packet;
	char *title = NULL;

	if (!title_arg) {
		title = strdup("nonull");
	} else {
		title = strdup(title_arg);
	}
	if (!title) {
		errorf("strdup failed\n");
		goto fail;
	}

	memset((void *)&address, 0, sizeof(struct sockaddr_in6));
	memset(&packet, 0, sizeof(struct _packet));

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

	if (sipc_send_packet(&packet, fd, identifier) == NOK) {
		errorf("sipc_send_packet() failed with %d: %s\n", errno, strerror(errno));
		goto fail;
	}

	if (!local_svr_port) {
		if (!callback) {
			errorf("this is the first time register call. 'callback' cannot be NULL\n");
			goto fail;
		}
		if (recv(fd, &local_svr_port, sizeof(unsigned int), 0) == -1) {
			errorf("recv() failed with %d: %s\n", errno, strerror(errno));
			goto fail;
		}

		if (local_svr_port < STARTING_PORT || local_svr_port > STARTING_PORT + BACKLOG) {
			errorf("port number is invalid\n");
			goto fail;
		}
		debugf("port %d initialized for this app\n", local_svr_port);
		create_server_thread(callback, local_svr_port);
	}
	*identifier = local_svr_port;

	goto out;

fail:
	ret = NOK;

out:
	close(fd);
	FREE(packet.title);
	FREE(packet.payload);
	FREE(title);

	return ret;
}

int sipc_register(_sipc_identifier *identifier, char *title, int (*callback)(void *, unsigned int))
{
	if (!title || !identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, callback, REGISTER, NULL, 0, PORT, identifier);
}

int sipc_broadcast(_sipc_identifier *identifier, void *data, unsigned int len)
{
	if (!data || !len || !identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(NULL, NULL, BROADCAST, data, len, PORT, identifier);
}

static int sipc_unregister_all(_sipc_identifier *identifier)
{
	char unreg_buf[256] = {0};

	if (!identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", *identifier);

	return sipc_send(NULL, NULL, UNREGISTER_ALL, unreg_buf, strlen(unreg_buf), PORT, identifier);
}

int sipc_destroy(_sipc_identifier *identifier)
{
	if (!identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	if (sipc_unregister_all(identifier) == NOK) {
		return NOK;
	}
	if (sipc_send(NULL, NULL, DESTROY, NULL, 0, *identifier, identifier) == NOK) {
		return NOK;
	}

	sleep(2);	//this is here for observing data release for valgrind

	return OK;
}

int sipc_unregister(_sipc_identifier *identifier, char *title)
{
	char unreg_buf[256] = {0};

	if (!title || !identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", *identifier);
	return sipc_send(title, NULL, UNREGISTER, unreg_buf, strlen(unreg_buf), PORT, identifier);
}

int sipc_send_data(_sipc_identifier *identifier, char *title, void *data, unsigned int len)
{
	if (!title || !data || !len || !identifier) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, NULL, SENDATA, data, len, PORT, identifier);
}
