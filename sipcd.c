#include "sipc_lib.h"

static struct title_list title_list;
static struct orphan_list orphan_list;

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

static void sigint_handler(__attribute__((unused)) int sig_num)
{
	title_data_structure_destroy(&title_list);
	orphan_data_structure_destroy(&orphan_list);

	exit(NOK);
}

int main(int argc, char **argv)
{
	int ret = OK;
	int c, o;

	signal(SIGINT, sigint_handler);

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

	TAILQ_INIT(&title_list);
	TAILQ_INIT(&orphan_list);

	sipc_create_server_daemon(&title_list, &orphan_list);

	goto out;

fail:
	ret = NOK;

out:
	title_data_structure_destroy(&title_list);
	orphan_data_structure_destroy(&orphan_list);

	return ret;
}
