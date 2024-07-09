#include "rdma_server.h"

struct ctrl *server_session;
struct ibv_comp_channel *cc;
struct rdma_event_channel *ec;
struct rdma_cm_id *listener;
int s_queue_ctr = 0;

char server_memory[PAGE_SIZE];
struct ibv_mr *server_mr;

extern int rdma_server_status;

static int on_connect_request(struct rdma_cm_id *id, struct rdma_conn_param *param)
{
	struct rdma_conn_param cm_params = {};
	struct ibv_device_attr attrs = {};
	struct queue *q = &server_session->queues[s_queue_ctr++];

	printf("%s\n", __func__);

	id->context = q;
	q->cm_id = id;

	if (!q->ctrl->dev) 
		TEST_NZ(rdma_create_device(q));
	TEST_NZ(rdma_create_queue(q, cc));
	TEST_NZ(ibv_query_device(q->ctrl->dev->verbs, &attrs));

	cm_params.initiator_depth = param->initiator_depth;
	cm_params.responder_resources = param->responder_resources;
	cm_params.rnr_retry_count = param->rnr_retry_count;
	cm_params.flow_control = param->flow_control;

	TEST_NZ(rdma_accept(q->cm_id, &cm_params));
	return 0;
}

static int on_connection_server(struct queue *q)
{
//	printf("%s: s_queue_ctr = %d\n", __func__, s_queue_ctr);
//
	struct mr_attr mr;

        printf("%s\n", __func__);
        TEST_NZ(rdma_server_create_mr(server_session->dev->pd));

        mr.addr = (uint64_t) server_mr->addr;
        mr.length = sizeof(struct mr_attr);
        mr.stag.lkey = server_mr->lkey;
        memcpy(server_memory, &mr, sizeof(struct mr_attr));

        TEST_NZ(rdma_send_wr(q, IBV_WR_SEND, &mr, NULL));
        TEST_NZ(rdma_poll_cq(q->cq, 1));

        q->ctrl->servermr.addr = (uint64_t) server_mr->addr;
        q->ctrl->servermr.length = sizeof(struct mr_attr);
        q->ctrl->servermr.stag.lkey = server_mr->lkey;

	return 1;
}

static int on_disconnect(struct queue *q)
{
	printf("%s\n", __func__);

	rdma_disconnect(q->cm_id);
	rdma_destroy_qp(q->cm_id);
	ibv_destroy_cq(q->cq);
	rdma_destroy_id(q->cm_id);
	rdma_destroy_event_channel(ec);
	ibv_dealloc_pd(server_session->dev->pd);
	free(server_session->dev);
	server_session->dev = NULL;
	return 1;
}

static int on_event(struct rdma_cm_event *event)
{
	struct queue *q = (struct queue *) event->id->context;
	printf("%s\n", __func__);

	switch (event->event) {
		case RDMA_CM_EVENT_CONNECT_REQUEST:
			return on_connect_request(event->id, &event->param.conn);
		case RDMA_CM_EVENT_ESTABLISHED:
			return on_connection_server(q);
		case RDMA_CM_EVENT_DISCONNECTED:
			return on_disconnect(q);
		case RDMA_CM_EVENT_REJECTED:
			printf("connect rejected\n");
			return 1;
		default:
			printf("unknown event: %s\n", rdma_event_str(event->event));
			return 1;
	}
}

int start_rdma_server(struct sockaddr_in s_addr)
{

	printf("s_addr\n");
//        print_sockaddr_in(s_addr);

	struct rdma_cm_event *event;
//	struct mr_attr mr;
	int i;

	TEST_NZ(rdma_alloc_session(&server_session));
	printf("%s: rdma_alloc_session\n", __func__);
	ec = rdma_create_event_channel();
	printf("%s: rdma_create_event_channel\n", __func__);
	TEST_NZ((ec == NULL));
	printf("%s: ec == NULL\n", __func__);
	TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));
	printf("%s: rdma_create_id\n", __func__);
	TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *) &s_addr));
	printf("%s: rdma_bind_addr\n", __func__);
	TEST_NZ(rdma_listen(listener, NUM_QUEUES + 1));
	printf("%s: rdma_listen\n", __func__);

//	for (i = 0; i < NUM_QUEUES; i++) {
	while (!rdma_get_cm_event(ec, &event)) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);

		if (on_event(&event_copy))
			break;
	}
//	}

//	TEST_NZ(rdma_create_mr(server_session->dev->pd));
//	mr.addr = (uint64_t) server_mr->addr;
//	mr.length = sizeof(struct mr_attr);
//	mr.stag.lkey = server_mr->lkey;
//	memcpy(server_memory, &mr, sizeof(struct mr_attr));
//
//	rdma_send_wr(&server_session->queues[0], IBV_WR_SEND, &mr, NULL);
//	rdma_poll_cq(server_session->queues[0].cq, 1);
//
//	server_session->servermr.addr = (uint64_t) server_mr->addr;
//	server_session->servermr.length = sizeof(struct mr_attr);
//	server_session->servermr.stag.lkey = server_mr->lkey;

	rdma_server_status = RDMA_CONNECT;

	while (!rdma_get_cm_event(ec, &event)) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);

		if (on_event(&event_copy))
			break;
	}

	return 0;
}

struct queue *get_queue_server(int idx)
{
	return &server_session->queues[idx];
}
