#ifndef _RDMA_SERVER_H
#define _RDMA_SERVER_H

#include "rdma_common.h"

int start_rdma_server(struct sockaddr_in s_addr);
struct queue *get_queue_server(int);

#endif
