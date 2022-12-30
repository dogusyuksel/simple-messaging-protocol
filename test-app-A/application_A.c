#include <sipc_lib.h>

_sipc_identifier identifier = IDENTIFIER_INIT;

int my_callback(void *prm, unsigned int len)
{
	printf("here is my callback: %s len: %d\n", (char *)prm, len);

	return OK;
}

static void sigint_handler(__attribute__((unused)) int sig_num)
{
	sipc_unregister(&identifier, "App_A_Registered_title");
	sipc_unregister(&identifier, "App_A_To_Itself");

	sipc_destroy(&identifier);

	exit(NOK);
}

int main(int argc, char **argv)
{
	int i = 0;
	char send_buf[254] = {0};

	UNUSED(argc);

	signal(SIGINT, sigint_handler);

	sipc_register(&identifier, "App_A_Registered_title", &my_callback);
	sipc_register(&identifier, "App_A_To_Itself", NULL);

	for (i = 0; i < 2; i++) {
		sleep(1);	//no need normally. To see smooth logs

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"BROADCAST data %d from %s", i, argv[0]);
		sipc_broadcast(&identifier, send_buf, strlen(send_buf));

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"data %d to myself", (10 - i));
		sipc_send_data(&identifier, "App_A_To_Itself", send_buf, strlen(send_buf));
	}

	while(1) {
		sleep(10); //wait forever
	}

	sipc_unregister(&identifier, "App_A_Registered_title");
	sipc_unregister(&identifier, "App_A_To_Itself");

	sipc_destroy(&identifier);

	return OK;
}