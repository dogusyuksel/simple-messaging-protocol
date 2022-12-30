#include "sipc_lib.h"

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

int sipc_send_packet_daemon(struct _packet *packet, int fd)
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

	if (packet->payload_size && packet->payload && send(fd, packet->payload, packet->payload_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	return OK;
}

int sipc_send_packet(struct _packet *packet, int fd, _sipc_identifier *identifier)
{
	unsigned int local_port = *identifier;

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

	if (packet->payload_size && packet->payload && send(fd, packet->payload, packet->payload_size, 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	if (send(fd, &local_port, sizeof(local_port), 0) == -1) {
		errorf("send() failed with %d: %s\n", errno, strerror(errno));
		return NOK;
	}

	return OK;
}

static char *packet_type_beautiy(enum _packet_type type)
{
	switch (type) {
	case REGISTER:
		return "REGISTER";
		break;
	case SUBSCRIBER:
		return "SUBSCRIBER";
		break;
	case UNREGISTER:
		return "UNREGISTER";
		break;
	case UNREGISTER_ALL:
		return "UNREGISTER_ALL";
		break;
	case DESTROY:
		return "DESTROY";
		break;
	case BROADCAST:
		return "BROADCAST";
		break;
	default:
		break;
	}

	return "error";
}

int sipc_read_data_daemon(int sockfd, struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports)
{
	int ret = OK;
	int byte_write;
	struct _packet packet;
	unsigned int old_port = 0;
	unsigned int next_port = 0;

	if (!title_list || !orphan_list || !available_ports) {
		errorf("arg is null\n");
		goto fail;
	}

	memset(&packet, 0, sizeof(struct _packet));

	errno = 0;
	if (read(sockfd, &packet.packet_type, sizeof(packet.packet_type)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}
	debugf("packet_type: %s\n", packet_type_beautiy((enum _packet_type)packet.packet_type));

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
	debugf("title: %s\n", packet.title);

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
	debugf("payload: %s\n", packet.payload);

proceed:
	errno = 0;
	if (read(sockfd, &old_port, sizeof(old_port)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}

	if (!old_port && packet.packet_type == REGISTER) {
		next_port = next_available_port(available_ports);
		byte_write = write(sockfd, &next_port, sizeof(next_port));
		if (byte_write != sizeof(next_port)) {
			errorf("Write error to socket %d.\n", sockfd);
			goto fail;
		}
		old_port = next_port;

		sleep(5); //give time to caller to create server thread
	}

	if (sipc_packet_handler_daemon(&packet, old_port, title_list, orphan_list, available_ports) == NOK) {
		errorf("sipc_packet_handler() failed\n");
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

int sipc_read_data(int sockfd,  int (*callback)(void *, unsigned int), bool *destoy)
{
	int ret = OK;
	struct _packet packet;

	memset(&packet, 0, sizeof(struct _packet));

	errno = 0;
	if (read(sockfd, &packet.packet_type, sizeof(packet.packet_type)) < 0) {
		errorf("read error from socket %d, errno: %d\n", sockfd, errno);
		goto fail;
	}
	debugf("packet_type: %s\n", packet_type_beautiy((enum _packet_type)packet.packet_type));

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
	debugf("title: %s\n", packet.title);

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
	debugf("payload: %s\n", packet.payload);

proceed:
	if (callback && packet.packet_type == SUBSCRIBER && packet.payload_size) {
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

int sipc_send_daemon(char *title_arg, enum _packet_type packet_type, void *data, unsigned int len, unsigned int _port)
{
	int ret = OK;
	int fd;
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

		debugf("send data %s to port %d\n", packet.payload, _port);
	}

	if (sipc_send_packet_daemon(&packet, fd) == NOK) {
		errorf("sipc_send_packet_daemon() failed with %d: %s\n", errno, strerror(errno));
		goto fail;
	}

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

int sipc_send(char *title_arg, int (*callback)(void *, unsigned int), enum _packet_type packet_type, void *data, unsigned int len, unsigned int _port, _sipc_identifier *identifier)
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
		errorf("title cannot be NULL\n");
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

	sleep(2);	//here for observing data release for valgrind

	return OK;
}

int sipc_unregister(_sipc_identifier *identifier, char *title)
{
	char unreg_buf[256] = {0};

	if (!title || !identifier) {
		errorf("title cannot be NULL\n");
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

	return sipc_send(title, NULL, SUBSCRIBER, data, len, PORT, identifier);
}


void sipc_create_server_daemon(struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports)
{
	int enable = 1;
	int listen_fd, conn_fd, max_fd, ret_val, i;
	struct sockaddr_storage client_addr, server_addr;
	char c_ip_addr[INET6_ADDRSTRLEN] = {0};
	fd_set backup_set, client_set;
	struct timeval tv;

	memset(c_ip_addr, 0, sizeof(c_ip_addr));
	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	if (sipc_fill_wildcard_sockstorage(PORT, AF_UNSPEC, &server_addr) != 0) {
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

			FD_SET(conn_fd, &backup_set);
			if (conn_fd > max_fd) {
				max_fd = conn_fd;
			}
			continue;
		}

		for (i = 0; i <= max_fd; i++) {
			if (FD_ISSET(i, &client_set) && i != listen_fd) {
				if (sipc_read_data_daemon(i, title_list, orphan_list, available_ports) == NOK) {
					errorf("sipc_read_data_daemon() failed\n");
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
}

void *sipc_create_server(void *arg)
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

static struct title_list_entry *find_entry_in_title_list(char *title, struct title_list *title_list)
{
	struct title_list_entry *entry = NULL;

	if (!title || !title_list) {
		errorf("args are wrong\n");
		return NULL;
	}

	TAILQ_FOREACH(entry, title_list, entries) {
		if (entry->title && strcmp(title, entry->title) == 0) {
			return entry;
		}
	}

	return NULL;
}

static bool is_port_int_the_list(unsigned int port, struct port_list *port_list)
{
	struct port_list_entry *entry = NULL;

	if (!port_list || port < STARTING_PORT || port > STARTING_PORT + BACKLOG) {
		errorf("args are wrong\n");
		return false;
	}

	TAILQ_FOREACH(entry, port_list, entries) {
		if (entry->port == port) {
			return true;
		}
	}

	return false;
}

static int add_new_entry_to_the_port_list(unsigned int port, struct port_list *port_list)
{
	struct port_list_entry *entry = (struct port_list_entry *)calloc(1, sizeof(struct port_list_entry));

	if (!port_list || port < STARTING_PORT || port > STARTING_PORT + BACKLOG) {
		errorf("args are wrong\n");
		return NOK;
	}

	if (!entry) {
		errorf("data is null\n");
		return NOK;
	}

	entry->port = port;

	TAILQ_INSERT_HEAD(port_list, entry, entries);

	return OK;
}

static int add_new_entry_to_title_list(char *title, unsigned int port, struct title_list *title_list)
{
	struct title_list_entry *entry = (struct title_list_entry *)calloc(1, sizeof(struct title_list_entry));

	if (!title || port < STARTING_PORT || port > STARTING_PORT + BACKLOG || !title_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	if (!entry) {
		errorf("data is null\n");
		return NOK;
	}

	entry->title = calloc(strlen(title) + 1, sizeof(char));
	if (!entry->title) {
		errorf("calloc fail\n");
		return NOK;
	}

	strcpy(entry->title, title);
	TAILQ_INIT(&(entry->port_list)); 

	TAILQ_INSERT_HEAD(title_list, entry, entries);

	if (is_port_int_the_list(port, &(entry->port_list)) == false) {
		if (add_new_entry_to_the_port_list(port, &(entry->port_list)) == NOK) {
			errorf("add_new_entry_to_the_port_list() failed\n");
			return NOK;
		}
	} else {
		debugf("port is already in the list, do nothing\n");
	}

	return OK;
}

static int add_port_title_couple(char *title, unsigned int port, struct title_list *title_list)
{
	struct title_list_entry *entry = NULL;

	if (!title || port < STARTING_PORT || port > STARTING_PORT + BACKLOG || !title_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	if ((entry = find_entry_in_title_list(title, title_list)) == NULL) {
		debugf("add new entry to the title list\n");
		return add_new_entry_to_title_list(title, port, title_list);
	}

	//it means, there is an entry already for the title.
	//extend its port list
	if (is_port_int_the_list(port, &(entry->port_list)) == false) {
		if (add_new_entry_to_the_port_list(port, &(entry->port_list)) == NOK) {
			errorf("add_new_entry_to_the_port_list() failed\n");
			return NOK;
		}
	} else {
		debugf("port is already in the list, do nothing\n");
	}

	return OK;
}

static void port_data_structure_destroy(struct port_list *port_list)
{
	struct port_list_entry *entry1 = NULL;
	struct port_list_entry *entry2 = NULL;

	if (!port_list) {
		errorf("data is null\n");
		return;
	}

	if (TAILQ_EMPTY(port_list)) {
		return;
	}

	entry1 = TAILQ_FIRST(port_list);
	while (entry1 != NULL) {
		entry2 = TAILQ_NEXT(entry1, entries);
		debugf("\tentry-port: %d\n", entry1->port);
		FREE(entry1);
		entry1 = entry2;
	}

	TAILQ_INIT(port_list); 
}

void title_data_structure_destroy(struct title_list *title_list)
{
	struct title_list_entry *entry1 = NULL;
	struct title_list_entry *entry2 = NULL;

	if (!title_list) {
		errorf("data is null\n");
		return;
	}

	if (TAILQ_EMPTY(title_list)) {
		return;
	}

	entry1 = TAILQ_FIRST(title_list);
	while (entry1 != NULL) {
		entry2 = TAILQ_NEXT(entry1, entries);
		debugf("entry-title: %s\n", entry1->title);
		FREE(entry1->title);
		port_data_structure_destroy(&(entry1->port_list));
		FREE(entry1);
		entry1 = entry2;
	}

	TAILQ_INIT(title_list); 
}

void orphan_data_structure_destroy(struct orphan_list *orphan_list)
{
	struct orphan_list_entry *entry1 = NULL;
	struct orphan_list_entry *entry2 = NULL;

	if (!orphan_list) {
		errorf("data is null\n");
		return;
	}

	if (TAILQ_EMPTY(orphan_list)) {
		return;
	}

	entry1 = TAILQ_FIRST(orphan_list);
	while (entry1 != NULL) {
		entry2 = TAILQ_NEXT(entry1, entries);
		FREE(entry1->title);
		FREE(entry1->data);
		FREE(entry1);
		entry1 = entry2;
	}

	TAILQ_INIT(orphan_list);
}

static int remove_port_from_title(char *title, unsigned int port, struct title_list *title_list)
{
	struct title_list_entry *entry = NULL;
	struct port_list_entry *pentry = NULL;

	if (!title || port < STARTING_PORT || port > STARTING_PORT + BACKLOG || !title_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	if ((entry = find_entry_in_title_list(title, title_list)) == NULL) {
		debugf("nothing to do\n");
		return OK;
	}

	TAILQ_FOREACH(pentry, &(entry->port_list), entries) {
		if (port == pentry->port) {
			TAILQ_REMOVE(&(entry->port_list), pentry, entries);
			FREE(pentry);
			break;
		}
	}

	return OK;
}

static int remove_port_from_all_title(unsigned int port, struct title_list *title_list)
{
	struct title_list_entry *entry = NULL;
	struct port_list_entry *pentry = NULL;

	if (port < STARTING_PORT || port > STARTING_PORT + BACKLOG || !title_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	TAILQ_FOREACH(entry, title_list, entries) {
		TAILQ_FOREACH(pentry, &(entry->port_list), entries) {
			if (port == pentry->port) {
				TAILQ_REMOVE(&(entry->port_list), pentry, entries);
				FREE(pentry);
			}
		}
	}

	return OK;
}

static int send_data_to_all_title(char *title, char *data, struct title_list *title_list)
{
	struct title_list_entry *entry = NULL;
	struct port_list_entry *pentry = NULL;

	if (!title || !title_list || !data) {
		errorf("args are wrong\n");
		return NOK;
	}

	if ((entry = find_entry_in_title_list(title, title_list)) == NULL) {
		debugf("nothing to do\n");
		return NOK;
	}

	TAILQ_FOREACH(pentry, &(entry->port_list), entries) {
		if (sipc_send_daemon(NULL, SUBSCRIBER, (void *)data, strlen(data), pentry->port) == NOK) {
			errorf("sipc_send() failed\n");
		}
	}

	return OK;
}

static int send_data_broadcast(char *data, struct title_list *title_list)
{
	bool local_port_buffer[BACKLOG] = {0};
	struct title_list_entry *entry = NULL;
	struct port_list_entry *pentry = NULL;

	if (!data || !title_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	memset(local_port_buffer, 0, sizeof(bool) * BACKLOG);

	TAILQ_FOREACH(entry, title_list, entries) {
		TAILQ_FOREACH(pentry, &(entry->port_list), entries) {
			if (!local_port_buffer[pentry->port - STARTING_PORT] &&
				sipc_send_daemon(NULL, SUBSCRIBER, (void *)data, strlen(data), pentry->port) == NOK) {
				errorf("sipc_send() failed\n");
				continue;
			}
			local_port_buffer[pentry->port - STARTING_PORT] = true;
		}
	}

	return OK;
}

static int add_data_to_orphan_list(char *title, char *data, struct orphan_list *orphan_list)
{
	int ret = OK;
	struct orphan_list_entry *entry = (struct orphan_list_entry *)calloc(1, sizeof(struct orphan_list_entry));

	if (!orphan_list || !title || !data) {
		errorf("args are wrong\n");
		return NOK;
	}

	if (!entry) {
		errorf("data is null\n");
		return NOK;
	}

	entry->data = strdup(data);
	if (!entry->data) {
		errorf("strdup failed\n");
		goto fail;
	}

	entry->title = strdup(title);
	if (!entry->title) {
		errorf("strdup failed\n");
		goto fail;
	}

	TAILQ_INSERT_HEAD(orphan_list, entry, entries);

	goto success;

fail:
	ret = NOK;

	FREE(entry->data);
	FREE(entry->title);
	FREE(entry);

success:

	return ret;
}

static void remove_sent_orphans_from_list(struct orphan_list *orphan_list)
{
	struct orphan_list_entry *entry = NULL;

	if (!orphan_list) {
		errorf("args are wrong\n");
		return;
	}

	TAILQ_FOREACH(entry, orphan_list, entries) {
		if (entry->is_sent) {
			TAILQ_REMOVE(orphan_list, entry, entries);
			FREE(entry->data);
			FREE(entry->title);
			FREE(entry);
			break;
		}
	}
}

static int send_orphan_data_first(char *title, unsigned int port, struct orphan_list *orphan_list)
{
	struct orphan_list_entry *entry = NULL;

	if (!title || port < STARTING_PORT || port > STARTING_PORT + BACKLOG || !orphan_list) {
		errorf("args are wrong\n");
		return NOK;
	}

	TAILQ_FOREACH(entry, orphan_list, entries) {
		if (entry->title && strcmp(title, entry->title) == 0) {
			if (entry->data) {
				if (sipc_send_daemon(NULL, SUBSCRIBER, (void *)entry->data, strlen(entry->data), port) == NOK) {
					errorf("sipc_send() failed\n");
				} else {
					entry->is_sent = true;
				}
			}
		}
	}

	remove_sent_orphans_from_list(orphan_list);

	return OK;
}

int sipc_packet_handler_daemon(struct _packet *packet, unsigned int port, struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports)
{
	int ret = OK;
	unsigned int lport = 0;
	char *ptr = NULL;

	if (!packet) {
		errorf("arg is null\n");
		goto fail;
	}

	switch (packet->packet_type) {
		case REGISTER:
			if (add_port_title_couple(packet->title, port, title_list) == NOK) {
				errorf("add_port_title_couple() failed\n");
				goto fail;
			}
			if (send_orphan_data_first(packet->title, port, orphan_list) == NOK) {
				errorf("send_orphan_data_first() failed\n");
				goto fail;
			}
			available_ports[port - STARTING_PORT] = true;
			break;
		case UNREGISTER:
			if (!packet->payload) {
				errorf("paload i null\n");
				goto fail;
			}

			lport = strtoul(packet->payload, &ptr, 10);
			if (remove_port_from_title(packet->title, lport, title_list) == NOK) {
				errorf("remove_port_from_title() failed\n");
				goto fail;
			}
			available_ports[lport - STARTING_PORT] = false;
			break;
		case UNREGISTER_ALL:
			if (!packet->payload) {
				errorf("paload i null\n");
				goto fail;
			}

			lport = strtoul(packet->payload, &ptr, 10);
			if (remove_port_from_all_title(lport, title_list) == NOK) {
				errorf("remove_port_from_all_title() failed\n");
				goto fail;
			}
			available_ports[lport - STARTING_PORT] = false;
			break;
		case SUBSCRIBER:
			if (send_data_to_all_title(packet->title, packet->payload, title_list) == NOK) {
				errorf("send_data_to_all_title() failed\n");
				if (add_data_to_orphan_list(packet->title, packet->payload, orphan_list) == NOK) {
					errorf("add_data_to_orphan() failed\n");
					goto fail;
				} else {
					debugf("data added to the orphan list\n");
				}
			}
			break;
		case BROADCAST:
			if (send_data_broadcast(packet->payload, title_list) == NOK) {
				errorf("send_data_to_all_title() failed\n");
				goto fail;
			}
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

unsigned int next_available_port(bool *available_ports)
{
	unsigned int i = 0;

	if (!available_ports) {
		return 0;
	}

	for (i = 0; i < BACKLOG; i++) {
		if (available_ports[i] == false) {
			return i + STARTING_PORT;
		}
	}

	return 0;
}
