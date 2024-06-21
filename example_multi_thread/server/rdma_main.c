#include "rdma_common.h"
#include "rdma_server.h"

pthread_t server_init;
pthread_t receiver[NUM_QUEUES];
pthread_t worker;

struct sockaddr_in s_addr;
int rdma_status;

atomic_int wr_check[100];

static void *process_server_init(void *arg)
{
	printf("%s: \n", __func__);
	rdma_status = RDMA_INIT;
	start_rdma_server(s_addr);
}

static void *process_receiver(void *arg)
{
	printf("%s: \n", __func__);
	int cpu = *(int *)arg;
	struct queue *q = get_queue(cpu);

	int cnt = 0;
	printf("%s: start on %d\n", __func__, cpu);

	while(true){
	   	printf("%s: recv %d!\n", __func__, cpu);
		rdma_recv_wr(q, &q->ctrl->servermr);
		rdma_poll_cq(q->cq, 1);
		printf("%s: done %d!\n", __func__, cpu);

		atomic_fetch_add(&wr_check[cnt], 1);
		cnt++;
	}
}

static void *process_worker(void *arg)
{
	printf("%s: \n", __func__);
        int idx = 0;

        while (true) {
                if (!atomic_load(&wr_check[idx]))
                        continue;

                printf("worker %d on!\n", idx);
                idx++;
        }
}


static void usage(void)
{
	printf("%s: \n", __func__);
	printf("[Usage] : ");
	printf("./rdma_main [-port <server port>]\n");
	exit(1);
}

int main(int argc, char* argv[])
{
	int ret, option;
	while ((option = getopt(argc, argv, "p:")) != -1) {
		switch (option) {
			case 'p':
				s_addr.sin_port = htons(strtol(optarg, NULL, 0));
				printf("%s: listening on port %s.\n", __func__, optarg);
				break;
			default:
				usage();
				break;
		}
	}

	if (!s_addr.sin_port) {
		usage();
		return ret;
	}
	s_addr.sin_family = AF_INET;

	pthread_create(&server_init, NULL, process_server_init, NULL);
	while (rdma_status != RDMA_CONNECT);

	printf("The server is connected successfully\n");

	sleep(2);

	pthread_create(&worker, NULL, process_worker, NULL);
//	pthread_create(&receiver, NULL, process_receiver, NULL);
	for (int i = 1; i < NUM_QUEUES+1; i++) {
//		pthread_create(&worker[i], NULL, process_worker, &i);
		printf("%s: before making receiver %d.\n", __func__, i);
		pthread_create(&receiver[i], NULL, process_receiver, &i);
		printf("%s: making receiver %d.\n", __func__, i);
//		sleep(1);
 	}

//	pthread_create(&receiver_manager, NULL, process_receiver_manager, NULL);

	pthread_join(server_init, NULL);
	printf("%s: It is OK!!!.\n", __func__);
	return ret;
}
