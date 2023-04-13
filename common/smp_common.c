#include "smp_common.h"

int smp_common_socket_open_use_buf(void)
{
	return socket(AF_INET6, SOCK_STREAM, 0);
}

int smp_common_fill_wildcard_sockstorage(unsigned short port, struct sockaddr_storage *addr)
{
	if (!addr) {
		return NOK;
	}

	return smp_common_buf_to_sockstorage(IPV6_WILDCARD_ADDR, port, addr);
}

int smp_common_buf_to_sockstorage(const char *addr, unsigned short port,
	struct sockaddr_storage *converted_addr)
{
	if (!converted_addr || !addr) {
		return NOK;
	}

	if (inet_pton(AF_INET6, addr, &(((struct sockaddr_in6 *)converted_addr)->sin6_addr)) > 0) {
		((struct sockaddr *)converted_addr)->sa_family = AF_INET6;
		((struct sockaddr_in6 *)converted_addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)converted_addr)->sin6_port = htons(port);
		return OK;
	}

	return NOK;
}

static int smp_common_accept_socket(int sockfd, struct sockaddr *addr)
{
	socklen_t size = sizeof(struct sockaddr_in6);

	if (!addr || sockfd < 0) {
		return NOK;
	}

	return accept(sockfd, addr, &size);

}

char *smp_common_packet_type_beauty(enum _packet_type type)
{
	switch (type) {
	case REGISTER:
		return "REGISTER";
		break;
	case SENDATA:
		return "SENDATA";
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
	default:
		break;
	}

	return "error";
}

int smp_common_socket_accept(int sockfd, struct sockaddr_storage *addr)
{
	int connfd;

	connfd = smp_common_accept_socket(sockfd, (struct sockaddr *)addr);
	if (connfd == -1) {
		errorf("Failed to accept: %s.\n", strerror(errno));
		return -1;
	}

	return connfd;
}

int smp_common_socket_open_use_sockaddr(const struct sockaddr *addr)
{
	if (!addr) {
		return NOK;
	}

	return socket(AF_INET6, SOCK_STREAM, 0);
}

int smp_common_bind_socket(int fd, const struct sockaddr *addr)
{
	if (!addr || fd < 0) {
		return NOK;
	}

	return bind(fd, addr, sizeof(struct sockaddr_in6));
}

int smp_common_connect_socket(int sockfd, const struct sockaddr *addr)
{
	if (!addr || sockfd < 0) {
		return NOK;
	}

	return connect(sockfd, addr, sizeof(struct sockaddr_in6));
}

int smp_common_socket_listen(int sockfd)
{
	if (listen(sockfd, BACKLOG) == -1) {
		return NOK;
	}

	return OK;
}
