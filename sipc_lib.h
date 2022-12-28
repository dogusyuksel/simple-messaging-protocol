#ifndef __SIPC_LIB_
#define __SIPC_LIB_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/queue.h>
#include <poll.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>


#define VERSION		"01.00"

#define UNUSED(__val__)		((void)__val__)

#define OK			0
#define NOK			1

#define RECEIVE_TIMEOUT	30

#define BUFFER_SIZE	1024

#define BACKLOG		254

#define PORT		    9191
#define STARTING_PORT   9192

#define IPV6_WILDCARD_ADDR		"::"
#define IPV6_LOOPBACK_ADDR		"::1"
#define IPV4_WILDCARD_ADDR		"0.0.0.0"
#define IPV4_LOOPBACK_ADDR		"127.0.0.1"
#define DEFAULT_LOCALHOST_IFACE	"lo"

#ifdef OPEN_DEBUG
#define debugf(...)		fprintf(stdout, __VA_ARGS__)
#else
#define debugf(...)		
#endif
#define errorf(...)		fprintf(stderr, __VA_ARGS__)

#define FREE(p)			{										\
							if (p) {							\
								free(p);						\
								p = NULL;						\
							}									\
						}

#define FCLOSE(p)		{										\
							if (p) {							\
								fflush(p);						\
								fclose(p);						\
								p = NULL;						\
							}									\
						}

enum _caller
{
	FROM_SERVER,
	FROM_CLIENT
};

enum _packet_type
{
	REGISTER,
	SUBSCRIBER,
	UNREGISTER
};

struct _packet
{
	unsigned char packet_type;
	unsigned int title_size;
	unsigned int port;
	char *title;
	unsigned int payload_size;
	char *payload;
};

struct _thread_data
{
	enum _caller caller;
	int (*callback)(void *, unsigned int);
	unsigned int port;
};

bool sipc_is_ipv6(const char *ipaddress);
bool sipc_ipv6_supported(void);
int sipc_buf_to_sockstorage(const char *addr, unsigned short port, struct sockaddr_storage *converted_addr);
int sipc_sockaddr_to_buf(const struct sockaddr *addr, char *conv_addr, size_t conv_addr_size);
int sipc_set_port(unsigned short port, struct sockaddr *sock);
int sipc_get_port(const struct sockaddr *sock);
int sipc_socket_open_use_buf(const char *buff, int scktype, int flag);
int sipc_socket_open_use_sockaddr(const struct sockaddr *addr, int scktype, int flag);
int sipc_bind_socket(int fd, const struct sockaddr *addr);
int sipc_connect_socket(int sockfd, const struct sockaddr *addr);
int sipc_accept_socket(int sockfd, struct sockaddr *addr);
int sipc_fill_wildcard_sockstorage(unsigned short port, unsigned int scktype, struct sockaddr_storage *addr);
int sipc_getnameinfo(const struct sockaddr *client_ip, char *ip, size_t sipc_size);
int sipc_socket_set_tos(int sockfd);
int sipc_socket_listen(int sockfd, int backlog);
int sipc_socket_accept(int sockfd, struct sockaddr_storage *addr);
int sipc_send_packet(struct _packet *packet, int fd);
int sipc_read_data(int sockfd,  int (*callback)(void *, unsigned int), enum _caller caller);
int sipc_send(const char *title, int (*callback)(void *, unsigned int), enum _packet_type packet_type, void *data, unsigned int len);
int sipc_register(const char *title, int (*callback)(void *, unsigned int));
int sipc_unregister(const char *title);
int spic_send(const char *title, void *data, unsigned int len);
void *sipc_create_server(void *arg);
int sipc_packet_handler(struct _packet *packet, unsigned int port);
unsigned int next_available_port(void);

#endif //__SIPC_LIB_
