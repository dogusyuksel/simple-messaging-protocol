#include "sipc_lib.h"

static unsigned int glb_port = 0;
static unsigned int glb_next_vailable_port = STARTING_PORT;

bool sipc_is_ipv6(const char *ipaddress)
{
	struct in6_addr r;

	if (!ipaddress) {
		return false;
	}

	memset(&r, 0, sizeof(struct sockaddr));
	if (inet_pton(AF_INET6, ipaddress, &r)) {
		return true;
	}

	return false;
}

bool sipc_ipv6_supported(void)
{
	int sfd = 0;

	sfd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (sfd == -1) {
		return false;
	}
	close(sfd);

	return true;
}

int sipc_buf_to_sockstorage(const char *addr, unsigned short port, struct sockaddr_storage *converted_addr)
{
	if (!converted_addr || !addr) {
		return NOK;
	}

	if (inet_pton(AF_INET, addr, &(((struct sockaddr_in *)converted_addr)->sin_addr)) > 0) {
		((struct sockaddr *)converted_addr)->sa_family = AF_INET;
		((struct sockaddr_in *)converted_addr)->sin_family = AF_INET;
		((struct sockaddr_in *)converted_addr)->sin_port = htons(port);
		return OK;
	}

	if (inet_pton(AF_INET6, addr, &(((struct sockaddr_in6 *)converted_addr)->sin6_addr)) > 0) {
		((struct sockaddr *)converted_addr)->sa_family = AF_INET6;
		((struct sockaddr_in6 *)converted_addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)converted_addr)->sin6_port = htons(port);
		return OK;
	}

	return NOK;
}

int sipc_sockaddr_to_buf(const struct sockaddr *addr, char *conv_addr, size_t conv_addr_size)
{
	if (!conv_addr || !addr) {
		return NOK;
	}

	switch (addr->sa_family) {
		case AF_INET:
			if (inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), conv_addr, conv_addr_size)) {
				return OK;
			}
			break;
		case AF_INET6:
			if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr), conv_addr, conv_addr_size)) {
				return OK;
			}
			break;
		default:
			break;
	}

	return NOK;
}

int sipc_set_port(unsigned short port, struct sockaddr *sock)
{
	if (!sock) {
		return NOK;
	}

	switch (sock->sa_family) {
		case AF_INET:
			((struct sockaddr_in *)sock)->sin_port = htons(port);
			return OK;
		case AF_INET6:
			((struct sockaddr_in6 *)sock)->sin6_port = htons(port);
			return OK;
		default:
			break;
	}

	return NOK;
}

int sipc_get_port(const struct sockaddr *sock)
{
	if (!sock) {
		return NOK;
	}

	switch (sock->sa_family) {
		case AF_INET:
			return ntohs(((struct sockaddr_in *)sock)->sin_port);
		case AF_INET6:
			return ntohs(((struct sockaddr_in6 *)sock)->sin6_port);
		default:
			break;
	}

	return NOK;
}

int sipc_socket_accept(int sockfd, struct sockaddr_storage *addr)
{
	int connfd;

	connfd = sipc_accept_socket(sockfd, (struct sockaddr *)addr);
	if (connfd == -1) {
		errorf("Failed to accept: %s.\n", strerror(errno));
		return -1;
	}

	(void) sipc_socket_set_tos(connfd);

	return connfd;
}

int sipc_socket_open_use_buf(const char *buff, int scktype, int flag)
{
	if (!buff) {
		return NOK;
	}

	if (sipc_is_ipv6(buff)) {
		return socket(AF_INET6, scktype, flag);
	}

	return socket(AF_INET, scktype, flag);
}

int sipc_socket_open_use_sockaddr(const struct sockaddr *addr, int scktype, int flag)
{
	if (!addr) {
		return NOK;
	}

	if (addr->sa_family == AF_INET6) {
		return socket(AF_INET6, scktype, flag);
	}

	return socket(AF_INET, scktype, flag);
}

int sipc_bind_socket(int fd, const struct sockaddr *addr)
{
	if (!addr || fd < 0) {
		return NOK;
	}

	return bind(fd, addr,
					(addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6));
}

int sipc_connect_socket(int sockfd, const struct sockaddr *addr)
{
	if (!addr || sockfd < 0) {
		return NOK;
	}

	return connect(sockfd, addr,
					(addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6));
}

int sipc_accept_socket(int sockfd, struct sockaddr *addr)
{
	socklen_t size = 0;

	if (!addr || sockfd < 0) {
		return NOK;
	}

	(addr->sa_family == AF_INET) ? (size = sizeof(struct sockaddr_in)) : (size = sizeof(struct sockaddr_in6));

	return accept(sockfd, addr, &size);

}

int sipc_fill_wildcard_sockstorage(unsigned short port, unsigned int scktype, struct sockaddr_storage *addr)
{
	if (!addr) {
		return NOK;
	}

	if (scktype == AF_UNSPEC) {
		if (sipc_ipv6_supported()) {
			return sipc_buf_to_sockstorage(IPV6_WILDCARD_ADDR, port, addr);
		}
		return sipc_buf_to_sockstorage(IPV4_WILDCARD_ADDR, port, addr);
	}

	if (sipc_ipv6_supported() && scktype == AF_INET6) {
		return sipc_buf_to_sockstorage(IPV6_WILDCARD_ADDR, port, addr);
	}

	return sipc_buf_to_sockstorage(IPV4_WILDCARD_ADDR, port, addr);
}

int sipc_getnameinfo(const struct sockaddr *client_ip, char *ip, size_t sipc_size)
{
	if (!client_ip || !ip) {
		return NOK;
	}

	if (client_ip->sa_family == AF_INET) {
		struct sockaddr_in *sa4 = (struct sockaddr_in*) client_ip;
		strncpy(ip, (char *)inet_ntoa(sa4->sin_addr), sipc_size);
		return OK;
	}

	if (client_ip->sa_family == AF_INET6) {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6*) client_ip;
		char buffer[INET6_ADDRSTRLEN];

		if (getnameinfo(client_ip, sizeof(struct sockaddr_in6), buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST) != 0) {
			return NOK;
		}

		if (IN6_IS_ADDR_V4MAPPED(&(sa6->sin6_addr))) {
			char *v4addr;

			v4addr = strrchr(buffer, ':') + 1;
			if (v4addr) {
				strncpy(ip, v4addr, sipc_size);
				return OK;
			}
		}

		strncpy(ip, buffer, sipc_size);
		return OK;
	}

	return NOK;
}


int sipc_socket_set_tos(int sockfd)
{
	int ret, tos_val = IPTOS_LOWDELAY; //0x10;	/* tos value depends on wireless queue handling */

	ret = setsockopt(sockfd, IPPROTO_IP, IP_TOS, (void *) &tos_val, sizeof(tos_val));
	if (ret == -1) {
		errorf("Failed to set tos value: %s.\n", strerror(errno));
		return NOK;
	}

	ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_TCLASS, (void *) &tos_val, sizeof(tos_val));
	if (ret == -1) {
		errorf("Failed to set tos value: %s.\n", strerror(errno));
		return NOK;
	}

	ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVTCLASS, (void *) &tos_val, sizeof(tos_val));
	if (ret == -1) {
		errorf("Failed to set tos value: %s.\n", strerror(errno));
		return NOK;
	}

	return OK;
}

int sipc_socket_listen(int sockfd, int backlog)
{
	int ret;

	(void) sipc_socket_set_tos(sockfd);

	ret = listen(sockfd, backlog);
	if (ret == -1) {
		errorf("Failed to listen on socket: %s.\n", strerror(errno));
		return NOK;
	}

	return OK;
}

int sipc_send_packet(struct _packet *packet, int fd)
{
	if (!packet || !packet->title || fd < 0) {
		errorf("arg wrong\n");
		return NOK;
	}

	if (send(fd, &packet->packet_type, sizeof(packet->packet_type), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (send(fd, &packet->title_size, sizeof(packet->title_size), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (packet->title_size && send(fd, packet->title, packet->title_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (send(fd, &packet->payload_size, sizeof(packet->payload_size), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (packet->payload_size && send(fd, packet->payload, packet->payload_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (send(fd, &glb_port, sizeof(glb_port), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	return OK;
}

int sipc_read_data(int sockfd,  int (*callback)(void *, unsigned int), enum _caller caller)
{
	int ret = OK;
	int byte_write;
	struct _packet packet;
	unsigned int old_port = 0;
	unsigned int next_port = 0;

	memset(&packet, 0, sizeof(struct _packet));

	errno = 0;
	if (read(sockfd, &packet.packet_type, sizeof(packet.packet_type)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}
	debugf("packet.packet_type: %d\n", packet.packet_type);

	errno = 0;
	if (read(sockfd, &packet.title_size, sizeof(packet.title_size)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}
	debugf("packet.title_size: %d\n", packet.title_size);

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
	debugf("packet.title: %s\n", packet.title);

	errno = 0;
	if (read(sockfd, &packet.payload_size, sizeof(packet.payload_size)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}
	debugf("packet.payload_size: %d\n", packet.payload_size);

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
	debugf("packet.payload: %s\n", packet.payload);

proceed:
	if (caller == FROM_SERVER) {
		errno = 0;
		if (read(sockfd, &old_port, sizeof(old_port)) < 0) {
			errorf("read error from socket %d, errno: %d\n", sockfd, errno);
			goto fail;
		}
		debugf("old_port: %d\n", old_port);

		if (!old_port && packet.packet_type == REGISTER) {
			next_port = next_available_port();
			byte_write = write(sockfd, &next_port, sizeof(next_port));
			if (byte_write != sizeof(next_port)) {
				errorf("Write error to socket %d.\n", sockfd);
				goto fail;
			}
			old_port = next_port;
		}

		if (sipc_packet_handler(&packet, old_port) == NOK) {
			errorf("sipc_packet_handler() failed\n");
			goto fail;
		}
	} else if (caller == FROM_CLIENT) {
		if (callback && packet.packet_type == SUBSCRIBER && packet.payload_size) {
			callback(packet.payload, packet.payload_size);
		}
	} else {
		errorf("caller is wrong\n");
		goto fail;
	}

	goto out;

fail:
	ret = NOK;

out:
	FREE(packet.title);
	FREE(packet.payload);

	return ret;
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

int sipc_send(const char *title, int (*callback)(void *, unsigned int), enum _packet_type packet_type, void *data, unsigned int len)
{
	int ret = OK;
	int fd;
	struct sockaddr_storage address;
	struct _packet packet;

	if (!title) {
		errorf("title cannot be NULL\n");
		return NOK;
	}

	memset((void *)&address, 0, sizeof(struct sockaddr_in6));
	memset(&packet, 0, sizeof(struct _packet));

	if (sipc_buf_to_sockstorage(IPV6_LOOPBACK_ADDR, PORT, &address) == NOK) {
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
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		goto fail;
	}

	if (!glb_port) {
		if (recv(fd, &glb_port, sizeof(unsigned int), 0) == -1) {
			errorf("recv() failed with %d: %s\n", errno, strerror(errno));
			goto fail;
		}
		debugf("glb_port: %d\n", glb_port);

		if (glb_port < STARTING_PORT || glb_port > STARTING_PORT + BACKLOG) {
			errorf("port number is invalid\n");
			goto fail;
		}
		debugf("port %d initialized for the title %s\n", glb_port, title);

		create_server_thread(callback, glb_port);
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

int sipc_register(const char *title, int (*callback)(void *, unsigned int))
{
	if (!title || !callback) {
		errorf("title cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, callback, REGISTER, NULL, 0);
}

int sipc_unregister(const char *title)
{
	char unreg_buf[256] = {0};

	if (!title) {
		errorf("title cannot be NULL\n");
		return NOK;
	}

	snprintf(unreg_buf, sizeof(unreg_buf), "%d", glb_port);
	return sipc_send(title, NULL, UNREGISTER, unreg_buf, strlen(unreg_buf));
}

int spic_send(const char *title, void *data, unsigned int len)
{
	if (!title || !data || !len) {
		errorf("args cannot be NULL\n");
		return NOK;
	}

	return sipc_send(title, NULL, SUBSCRIBER, data, len);
}


void *sipc_create_server(void *arg)
{
	struct _thread_data *thdata = arg;
	int enable = 1;
	int listen_fd, conn_fd, max_fd, ret_val, i;
	struct sockaddr_storage client_addr, server_addr;
	char c_ip_addr[INET6_ADDRSTRLEN] = {0};
	fd_set backup_set, client_set;
	struct timeval tv;
	enum _caller caller = FROM_CLIENT;

	if (!thdata) {
		caller = FROM_SERVER;
	}

	memset(c_ip_addr, 0, sizeof(c_ip_addr));
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	if (sipc_fill_wildcard_sockstorage(thdata ? thdata->port : PORT, AF_UNSPEC, &server_addr) != 0) {
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

	for (;;) {
		tv.tv_sec = RECEIVE_TIMEOUT;
		memcpy(&client_set, &backup_set, sizeof(backup_set));

		ret_val = select(max_fd + 1, &client_set, NULL, NULL, &tv);

		if (ret_val < 0) {
			errorf("select error\n");
			continue;
		} else if (ret_val == 0) {
			debugf("select timeout\n");
			for (i = 0; i <= max_fd; i++) {
				if (FD_ISSET(i, &backup_set) && i != listen_fd) {
						debugf("close connection %d\n", i);
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
				errorf("accept error.\n");
				goto out;
			}
			if (sipc_getnameinfo((struct sockaddr *)&client_addr, c_ip_addr, sizeof(c_ip_addr)) == 0) {
				debugf("connected client: %s:%d\n", c_ip_addr, conn_fd);
			}
			FD_SET(conn_fd, &backup_set);
			if (conn_fd > max_fd) {
				max_fd = conn_fd;
			}
			continue;
		}

		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i, &client_set) && i != listen_fd) {
				debugf("read from client %d\n", i);
				if (sipc_read_data(i,  thdata ? thdata->callback : NULL, caller) == NOK) {
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

	if (caller == FROM_CLIENT) {
		pthread_exit(NULL);
	}

	return NULL;
}

int sipc_packet_handler(struct _packet *packet, unsigned int port)
{
	int ret = OK;

	UNUSED(port);

	if (!packet) {
		errorf("arg is null\n");
		goto fail;
	}

	switch (packet->packet_type) {
		case REGISTER:
			debugf("TODO, add %d port for title %s to our queue\n", port, packet->title);
			break;
		case UNREGISTER:
			debugf("TODO, remove %s port for title %s to our queue\n", packet->payload, packet->title);
			break;
		case SUBSCRIBER:
			debugf("TODO, send %s data for title %s to our queue\n", packet->payload, packet->title);
			break;
		
		default:
			break;
	}

	goto out;

fail:
	ret = NOK;

out:
	return ret;
}

unsigned int next_available_port(void)
{
	glb_next_vailable_port++;
	return glb_next_vailable_port - 1;
}
