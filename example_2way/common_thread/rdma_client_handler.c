#include "rdma_common.h"
#include "rdma_client.h"

pthread_t client_init;
pthread_t c_worker;

struct sockaddr_in c_addr;
int rdma_client_status;
//extern struct ctrl client_session;

static void *process_client_init(void *arg)
{
        rdma_client_status = RDMA_INIT;
        start_rdma_client(&c_addr);
}

static void *simulator(void *arg)
{
	struct queue *q = get_queue_client(0);

	printf("%s: start\n", __func__);

	int i = 0;
	while(true){
		printf("%s: req %d\n", __func__, i);
		//@delee
		//TODO
		//Check '&q->ctrl->servermr'
//                rdma_send_wr(q, IBV_WR_SEND, &q->ctrl->servermr, NULL);
		rdma_send_wr(q, IBV_WR_SEND, &q->ctrl->clientmr, NULL);
		printf("%s: &q->ctrl->servermr %p\n", __func__, &q->ctrl->servermr);
		printf("%s: rdma_send_wr on %d\n", __func__, i);
                rdma_poll_cq(q->cq, 1);
                printf("%s: %d done\n", __func__, i);
		i++;
		if(i ==100){
			break;
			printf("%s: i is 100 in while \n", __func__);
		}
        }
	printf("%s: end of while \n", __func__);

	rdma_client_status = RDMA_DISCONNECT;
}

void *client_handler()
{
//	sleep(100);
	pthread_create(&client_init, NULL, process_client_init, NULL);
	printf("%s: Waiting for client is connected\n", __func__);
	while (rdma_client_status != RDMA_CONNECT);
	printf("%s: The client is connected successfully\n", __func__);

	sleep(2);

	pthread_create(&c_worker, NULL, simulator, NULL);
	pthread_join(client_init, NULL);
	while (rdma_client_status == RDMA_CONNECT);
}


