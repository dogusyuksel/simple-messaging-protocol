#include <sipc_lib.h>

int my_callback(void *prm, unsigned int len)
{
	printf("here is %s: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}

int my_callback2(void *prm, unsigned int len)
{
	printf("here is %s: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}

static void sigint_handler(__attribute__((unused)) int sig_num)
{
	sipc_unregister("App_A_Registered_title");
	sipc_unregister("App_A_To_Itself");

	sipc_destroy();

	exit(NOK);
}

int main(int argc, char **argv)
{
	int i = 0;
	char send_buf[254] = {0};

	UNUSED(argc);
	UNUSED(argv);

	signal(SIGINT, sigint_handler);

	sipc_register("App_A_Registered_title", &my_callback);
	sipc_register("App_A_To_Itself", &my_callback2);

	for (i = 0; i < 2; i++) {
		sleep(1);	//no need normally. To see smooth logs

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"data %d to myself", (10 - i));
		sipc_send_data("App_A_To_Itself", send_buf, strlen(send_buf));
	}

	while(1) {
		sleep(10); //wait forever
	}

	sipc_unregister("App_A_Registered_title");
	sipc_unregister("App_A_To_Itself");

	sipc_destroy();

	return OK;
}