#include "../sipc_lib.h"


int my_callback(void *prm, unsigned int len)
{
	printf("here is my cllback: %s len: %d\n", (char *)prm, len);

	return OK;
}

int main(int argc, char **argv)
{
	int i = 0;
	char send_buf[254] = {0};

	UNUSED(argc);
	UNUSED(argv);

	sipc_register("deneme", &my_callback);
	sipc_register("deneme2", &my_callback);

	for (i = 0; i < 10; i++) {
		sleep(2);
		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"\"deneme title data %d\"", i);
		spic_send("deneme", send_buf, strlen(send_buf));

		memset(send_buf, 0, sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), (char *)"\"deneme2 title data %d\"", i);
		spic_send("deneme2", send_buf, strlen(send_buf));
	}

	sipc_unregister("deneme");
	sipc_unregister("deneme2");

	return OK;
}