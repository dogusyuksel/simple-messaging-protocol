#include <sipc_lib.h>

int my_callback(void *prm, unsigned int len)
{
	printf("here is %s callback: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}
int bcast_callback(void *prm, unsigned int len)
{
	printf("here is %s callback: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}

static void sigint_handler(__attribute__((unused)) int sig_num)
{
	sipc_unregister("App_B_To_Itself");

	sipc_destroy();

	exit(NOK);
}

int main(int argc, char **argv)
{
	int i = 0;
	char send_buf[254] = {0};

	UNUSED(argc);

	signal(SIGINT, sigint_handler);

	if (sipc_register("App_B_To_Itself", &my_callback) == NOK) {
		errorf("sipc_register() failed\n");
		goto out;
	}
	if (sipc_broadcast_register(&bcast_callback) == NOK) {
		errorf("sipc_broadcast_register() failed\n");
		goto out;
	}

	for (i = 0; i < 2; i++) {
		sleep(1);	//no need normally. To see smooth logs

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"Hello AppA, here is your data %d from %s", i, argv[0]);
		if (sipc_send_data("App_A_Registered_title", send_buf, strlen(send_buf)) == NOK) {
			errorf("sipc_send_data() failed\n");
			goto out;
		}

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"data %d to myself", (10 - i));
		if (sipc_send_data("App_B_To_Itself", send_buf, strlen(send_buf)) == NOK) {
			errorf("sipc_send_data() failed\n");
			goto out;
		}
	}

	while(1) {
		sleep(10); //wait forever
	}

out:
	sipc_unregister("App_B_To_Itself");
	sipc_broadcast_unregister();

	sipc_destroy();

	return OK;
}
