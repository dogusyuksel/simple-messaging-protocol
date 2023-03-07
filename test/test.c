#include <sipc_lib.h>

int my_callback(void *prm, unsigned int len)
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
	char *saveptr = NULL, *token = NULL;

	if (argc <= 1) {
		printf("please type at least 1 arg which will be the title to be registered!\n");
		goto exit;
	}

	signal(SIGINT, sigint_handler);

	for ( i = 1; i < argc; i++) {
		if (sipc_register(argv[i], &my_callback, 10) == NOK) {
			printf("sipc_register() failed\n");
			goto out;
		}
		printf("registered to the title: %s\n", argv[i]);
	}

	for (i = 0; i < 2; i++) {
		sleep(1);	//no need normally. To see smooth logs

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"data %d to myself", (10 - i));
		if (sipc_send_data("App_A_To_Itself", send_buf, strlen(send_buf)) == NOK) {
			printf("sipc_send_data() failed\n");
			goto out;
		}
		snprintf(send_buf, sizeof(send_buf), (char *)"bcast data %d", i);
		if (sipc_send_bradcast_data(send_buf, strlen(send_buf)) == NOK) {
			printf("sipc_send_data() failed\n");
			goto out;
		}
	}

	while(1) {
		printf("Enter \"title data\" pair to send the other application:\n");

		if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
			printf("fgets failed\n");
			continue;
		}
		send_buf[strlen(send_buf) - 1] = '\0';

		token = strtok_r(send_buf, " ", &saveptr);

		if (sipc_send_data(token, saveptr, strlen(saveptr)) == NOK) {
			printf("sipc_send_data() failed\n");
			goto out;
		}
	}

out:
	for ( i = 1; i < argc; i++) {
		sipc_unregister(argv[i]);
	}

	sipc_destroy();

exit:
	return OK;
}
