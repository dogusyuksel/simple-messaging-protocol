#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <sys/queue.h>
#include <limits.h>
#include <poll.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <netdb.h>

#include "sipc_lib.h"

static struct option parameters[] = {
	{ "help",				no_argument,		0,	0x100	},
	{ "version",			no_argument,		0,	0x101	},
	{ NULL,					0,					0, 	0 		},
};

static void print_help_exit (void)
{
	debugf("Please refer to Readme.md\n");

	exit(OK);
}

int main(int argc, char **argv)
{
	int ret = OK;
	int c, o;

	while ((c = getopt_long(argc, argv, "h", parameters, &o)) != -1) {
		switch (c) {
			case 0x100:
				print_help_exit();
				break;
			case 0x101:
				debugf("%s version %s\n", argv[0], VERSION);
				return OK;
				break;
			default:
				debugf("unknown argument\n");
				goto fail;
		}
	}

	sipc_create_server(NULL);

	goto out;

fail:
	ret = NOK;

out:

	return ret;
}