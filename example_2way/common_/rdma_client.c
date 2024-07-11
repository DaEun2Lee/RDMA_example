#include "rdma_client.h"

struct ctrl *client_session;
struct ibv_comp_channel *c_cc[NUM_QUEUES];
struct rdma_event_channel *c_ec[NUM_QUEUES];
struct rdma_cm_id *cm_id[NUM_QUEUES];
int c_queue_ctr = 0;

char client_memory[PAGE_SIZE]; 
struct ibv_mr *client_mr;

extern int rdma_client_status;

static int on_addr_resolved(struct rdma_cm_id *id)
{
	struct queue *q = &client_session->queues[c_queue_ctr];
	printf("%s\n", __func__);

	id->context = q;
	q->cm_id = id;

	if (!q->ctrl->dev) 
		TEST_NZ(rdma_create_device(q));
	TEST_NZ(rdma_create_queue(q, c_cc[c_queue_ctr++]));
//	TEST_NZ(rdma_modify_qp(q));
	TEST_NZ(rdma_resolve_route(q->cm_id, CONNECTION_TIMEOUT_MS));
	return 0;
}

static int on_route_resolved(struct queue *q)
{
	struct rdma_conn_param param = {};

	printf("%s\n", __func__);

	param.qp_num = q->qp->qp_num;
	param.initiator_depth = 16;
	param.responder_resources = 16;
	param.retry_count = 7;
	param.rnr_retry_count = 7;
	TEST_NZ(rdma_connect(q->cm_id, &param));
	return 0;
}

static int on_connection_client(struct queue *q)
{
	struct mr_attr mr;

//	printf("%s: c_queue_ctr = %d\n", __func__, c_queue_ctr);
//	if (c_queue_ctr != NUM_QUEUES)
//		return 1;

	printf("%s\n", __func__);

	TEST_NZ(rdma_client_create_mr(client_session->dev->pd));

	mr.addr = (uint64_t) client_mr->addr;
	mr.length = sizeof(struct mr_attr);
	mr.stag.lkey = client_mr->lkey;
	memcpy(client_memory, &mr, sizeof(struct mr_attr));
	
	TEST_NZ(rdma_recv_wr(&client_session->queues[c_queue_ctr-1], &mr));
	TEST_NZ(rdma_poll_cq(client_session->queues[c_queue_ctr-1].cq, 1));
	return 1;
}

static int on_event(struct rdma_cm_event *event)
{
	struct queue *q = (struct queue *) event->id->context;
	printf("%s\n", __func__);

	switch (event->event) {
		case RDMA_CM_EVENT_ADDR_RESOLVED:
			return on_addr_resolved(event->id);
		case RDMA_CM_EVENT_ROUTE_RESOLVED:
			return on_route_resolved(q);
		case RDMA_CM_EVENT_ESTABLISHED:
			return on_connection_client(q);
		case RDMA_CM_EVENT_REJECTED:
			printf("%s: RDMA_CM_EVENT_REJECTD\n", __func__);
			return 1;
		case RDMA_CM_EVENT_DISCONNECTED:
			printf("%s: disconnect\n", __func__);
			return 1;
		default:
			printf("%s: %s\n", __func__, rdma_event_str(event->event));
			return 1;
	}
}

int start_rdma_client(struct sockaddr_in *c_addr)
{
	struct rdma_cm_event *event;

	TEST_NZ(rdma_alloc_session(&client_session));

	for (unsigned int i = 0; i < NUM_QUEUES; i++) {
		c_ec[i] = rdma_create_event_channel();
		TEST_NZ((c_ec[i] == NULL));
		TEST_NZ(rdma_create_id(c_ec[i], &cm_id[i], NULL, RDMA_PS_TCP));
		TEST_NZ(rdma_resolve_addr(cm_id[i], NULL, (struct sockaddr *) c_addr, 2000));

		while (!rdma_get_cm_event(c_ec[i], &event)) {
			struct rdma_cm_event event_copy;

			memcpy(&event_copy, event, sizeof(*event));
			rdma_ack_cm_event(event);

			if (on_event(&event_copy))
				break;
		}
	}

	rdma_client_status = RDMA_CONNECT;
	return 0;
}

struct queue *get_queue_client(int idx)
{
	return &client_session->queues[idx];
}
