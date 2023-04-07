#include <sipc_lib.h>

int my_callback(void *prm, unsigned int len)
{
	printf("here is %s: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}

static void sigint_handler(__attribute__((unused)) int sig_num)
{
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
