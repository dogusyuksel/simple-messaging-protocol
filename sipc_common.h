#ifndef __SIPC_COMMON_
#define __SIPC_COMMON_

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
#include <signal.h>

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

#define IDENTIFIER_INIT			0

#define ANSI_COLOR_RED		"\x1b[31m"
#define ANSI_COLOR_GREEN	"\x1b[32m"
#define ANSI_COLOR_YELLOW	"\x1b[33m"
#define ANSI_COLOR_BLUE		"\x1b[34m"
#define ANSI_COLOR_MAGENTA	"\x1b[35m"
#define ANSI_COLOR_CYAN		"\x1b[36m"
#define ANSI_COLOR_RESET	"\x1b[0m"

#ifdef OPEN_DEBUG
#define debugf(...)		fprintf(stderr, "[%d]\t", __LINE__);fprintf(stdout, __VA_ARGS__)
#else
#define debugf(...)		
#endif
#define errorf(...)		fprintf(stderr, ANSI_COLOR_RED"[%d]\t", __LINE__);fprintf(stderr, __VA_ARGS__);fprintf(stderr, ANSI_COLOR_RESET)

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

typedef unsigned int _sipc_identifier;

enum _packet_type
{
	REGISTER,
	SENDATA,
	UNREGISTER,
	UNREGISTER_ALL,
	DESTROY,
	BROADCAST
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

int sipc_socket_open_use_buf(const char *buff, int scktype, int flag);
int sipc_fill_wildcard_sockstorage(unsigned short port, unsigned int scktype,
    struct sockaddr_storage *addr);
int sipc_buf_to_sockstorage(const char *addr, unsigned short port,
    struct sockaddr_storage *converted_addr);
int sipc_socket_accept(int sockfd, struct sockaddr_storage *addr);
int sipc_socket_open_use_sockaddr(const struct sockaddr *addr, int scktype, int flag);
int sipc_bind_socket(int fd, const struct sockaddr *addr);
int sipc_connect_socket(int sockfd, const struct sockaddr *addr);
int sipc_socket_listen(int sockfd, int backlog);
char *packet_type_beautiy(enum _packet_type type);

#endif //__SIPC_COMMON