#include <libsmp.h>

int my_callback(void *prm, unsigned int len)
{
	printf("here is %s: %s len: %d\n", __func__, (char *)prm, len);

	return OK;
}

static void sigint_handler(__attribute__((unused)) int sig_num)
{
	smp_destroy();

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
		if (smp_register(argv[i], &my_callback, 10) == NOK) {
			printf("smp_register() failed\n");
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

		if (smp_send_data(token, saveptr, 10) == NOK) {
			printf("smp_send_data() failed\n");
			goto out;
		}
	}

out:
	for ( i = 1; i < argc; i++) {
		smp_unregister(argv[i]);
	}

	smp_destroy();

exit:
	return OK;
}
