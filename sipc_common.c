#include "sipc_common.h"

static bool sipc_is_ipv6(const char *ipaddress)
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

static bool sipc_ipv6_supported(void)
{
	int sfd = 0;

	sfd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (sfd == -1) {
		return false;
	}
	close(sfd);

	return true;
}

int sipc_fill_wildcard_sockstorage(unsigned short port, unsigned int scktype, struct sockaddr_storage *addr)
{
	bool ipv6_supported = sipc_ipv6_supported();
	if (!addr) {
		return NOK;
	}

	if (scktype == AF_UNSPEC) {
		if (ipv6_supported) {
			return sipc_buf_to_sockstorage(IPV6_WILDCARD_ADDR, port, addr);
		}
		return sipc_buf_to_sockstorage(IPV4_WILDCARD_ADDR, port, addr);
	}

	if (ipv6_supported && scktype == AF_INET6) {
		return sipc_buf_to_sockstorage(IPV6_WILDCARD_ADDR, port, addr);
	}

	return sipc_buf_to_sockstorage(IPV4_WILDCARD_ADDR, port, addr);
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

static int sipc_accept_socket(int sockfd, struct sockaddr *addr)
{
	socklen_t size = 0;

	if (!addr || sockfd < 0) {
		return NOK;
	}

	(addr->sa_family == AF_INET) ? (size = sizeof(struct sockaddr_in)) : (size = sizeof(struct sockaddr_in6));

	return accept(sockfd, addr, &size);

}

static int sipc_socket_set_tos(int sockfd)
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

char *packet_type_beautiy(enum _packet_type type)
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
	case BROADCAST:
		return "BROADCAST";
		break;
	default:
		break;
	}

	return "error";
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

int sipc_socket_listen(int sockfd, int backlog)
{
	int ret;

	(void) sipc_socket_set_tos(sockfd);

	ret = listen(sockfd, backlog);
	if (ret == -1) {
		return NOK;
	}

	return OK;
}
