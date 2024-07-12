#include "rdma_common.h"
#include "rdma_client.h"
#include "rdma_client_handler.h"
#include "rdma_server.h"
#include "rdma_server_handler.h"

extern struct sockaddr_in s_addr;
extern struct sockaddr_in c_addr;

pthread_t s_handler;
pthread_t c_handler;

static inline int get_addr(char *sip)
{
	struct addrinfo *info;

	TEST_NZ(getaddrinfo(sip, NULL, NULL, &info));
	memcpy(&c_addr, info->ai_addr, sizeof(struct sockaddr_in));
	freeaddrinfo(info);
	return 0;
}

static void usage(void)
{
	printf("[Usage] : ");
	printf("./rdma_main [-i <server ip>] [-p <server port>]\n");
}

int main(int argc, char* argv[])
{
	int option;

	while ((option = getopt(argc, argv, "i:p:P:")) != -1) {
		switch (option) {
			case 'i':
				TEST_NZ(get_addr(optarg));
				break;
			case 'p':
				s_addr.sin_port = htons(strtol(optarg, NULL, 0));
				c_addr.sin_port = s_addr.sin_port;
				break;
			//@delee
			//'P' is a client port.
			case 'P':
                                c_addr.sin_port = htons(strtol(optarg, NULL, 0));
                                break;
			default:
				usage();
				break;
		}
	}

	s_addr.sin_family = AF_INET;

	printf("s_addr\n");
	print_sockaddr_in(&s_addr);
	printf("c_addr\n");
	print_sockaddr_in(&c_addr);
//	server_handler();
//	sleep(2);
//	client_handler();
	if (pthread_create(&s_handler, NULL, server_handler, NULL) != 0) {
		printf("%s: Failed to create pthread_create-server_handler\n", __func__);
		return 1;
	}

	if (pthread_create(&c_handler, NULL, client_handler, NULL) != 0) {
		printf("%s: Failed to create pthread_create-client_handler\n", __func__);
	}
//	printf("%s: pthread_create-server_handler\n", __func__);
//	sleep(60);
// 	pthread_create(&c_handler, NULL, client_handler, NULL);
//	printf("%s: pthread_create-client_handler\n", __func__);
	pthread_join(s_handler, NULL);
//	sleep(5);
	pthread_join(c_handler, NULL);

	return 0;
}
