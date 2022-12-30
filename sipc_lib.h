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
#include <signal.h>


#define VERSION		"00.01"

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
	SUBSCRIBER,
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

struct _thread_data
{
	int (*callback)(void *, unsigned int);
	unsigned int port;
};

struct port_list_entry {
	unsigned int port;
	TAILQ_ENTRY(port_list_entry) entries;
};
TAILQ_HEAD(port_list, port_list_entry);

struct title_list_entry {
	char *title;
	struct port_list port_list;
	TAILQ_ENTRY(title_list_entry) entries;
};
TAILQ_HEAD(title_list, title_list_entry);

struct orphan_list_entry {
	char *title;
	char *data;
	bool is_sent;
	TAILQ_ENTRY(orphan_list_entry) entries;
};
TAILQ_HEAD(orphan_list, orphan_list_entry);

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
int sipc_send_packet(struct _packet *packet, int fd, _sipc_identifier *identifier);
int sipc_send_packet_daemon(struct _packet *packet, int fd);
int sipc_read_data(int sockfd,  int (*callback)(void *, unsigned int), bool *destoy);
int sipc_read_data_daemon(int sockfd, struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports);
int sipc_send(char *title, int (*callback)(void *, unsigned int), enum _packet_type packet_type, void *data, unsigned int len, unsigned int _port, _sipc_identifier *identifier);
int sipc_send_daemon(char *title_arg, enum _packet_type packet_type, void *data, unsigned int len, unsigned int _port);
void *sipc_create_server(void *arg);
void sipc_create_server_daemon(struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports);
int sipc_packet_handler_daemon(struct _packet *packet, unsigned int port, struct title_list *title_list, struct orphan_list *orphan_list, bool *available_ports);
unsigned int next_available_port(bool *available_ports);
void title_data_structure_destroy(struct title_list *title_list);
void orphan_data_structure_destroy(struct orphan_list *orphan_list);

int sipc_register(_sipc_identifier *identifier, char *title, int (*callback)(void *, unsigned int));
int sipc_broadcast(_sipc_identifier *identifier, void *data, unsigned int len);
int sipc_unregister(_sipc_identifier *identifier, char *title);
int sipc_destroy(_sipc_identifier *identifier);
int sipc_send_data(_sipc_identifier *identifier, char *title, void *data, unsigned int len);

#endif //__SIPC_LIB_
