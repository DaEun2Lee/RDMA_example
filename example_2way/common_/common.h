#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>

#define true 1
#define false 0

#define CONNECTION_TIMEOUT_MS 2000

#define CQ_CAPACITY (128)
#define MAX_SGE (32)
#define MAX_WR (64)

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

struct rdma_buffer_attr {
	uint64_t address;
	uint64_t length;
	union stag {
		uint32_t lkey;
		uint32_t rkey;
	} stag;
};

struct device {
	struct ibv_pd *pd;
	struct ibv_context *verbs;
};

struct queue {
	struct ibv_qp *qp;
	struct ibv_cq *cq;
	struct rdma_cm_id *cm_id;
	struct ctrl *ctrl;
	enum {
		INIT,
		CONNECTED
	} state;
};

struct ctrl {
	struct queue *queues;
	struct ibv_mr *mr_buffer;
	void *buffer;
	struct device *dev;

	struct ibv_comp_channel *comp_channel;
};

#endif
