#include "rdma_common.h"
#include "rdma_server.h"

pthread_t server_init;
pthread_t receiver;
pthread_t s_worker;

struct sockaddr_in s_addr;
//int rdma_server_status;
int rdma_server_status;

atomic_int wr_check[100];


//static void *process_server_init(void *arg)
static void *process_server_init()
{
        rdma_server_status = RDMA_INIT;
        start_rdma_server(s_addr);
	printf("%s: Complete start_rdma_server",__func__);
}

static void *process_receiver(void *arg)
{
//      int cpu = *(int *)arg;
//      struct queue *q = get_queue(cpu);
        struct queue *q = get_queue_server(0);
//
        int cnt = 0;
//      printf("%s: start on %d\n", __func__, cpu);

//      for (int i = 0; i < 100; i++) {
        while(true){
//              printf("%s: recv %d on %d!\n", __func__, i, cpu);
                rdma_recv_wr(q, &q->ctrl->clientmr);
//		rdma_recv_wr(q, &q->ctrl->servermr);
                rdma_poll_cq(q->cq, 1);
//              printf("%s: done %d on %d!\n", __func__, i, cpu);

                atomic_fetch_add(&wr_check[cnt], 1);
                cnt++;
        }
}

//
static void *process_worker(void *arg)
{
        int idx = 0;

        while (true) {
                if (!atomic_load(&wr_check[idx]))
                        continue;

                printf("worker %d on!\n", idx);
                idx++;
        }
}

void *server_handler()
{

	pthread_create(&server_init, NULL, process_server_init, NULL);
	while (rdma_server_status != RDMA_CONNECT);
	printf("The server is connected successfully\n");

	sleep(2);
	pthread_create(&s_worker, NULL, process_worker, NULL);
	pthread_create(&receiver, NULL, process_receiver, NULL);

	pthread_join(server_init, NULL);
	while (rdma_server_status == RDMA_CONNECT);
}



