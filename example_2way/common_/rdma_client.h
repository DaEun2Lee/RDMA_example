#ifndef _RDMA_CLIENT_H
#define _RDMA_CLIENT_H

#include "rdma_common.h"

int start_rdma_client(struct sockaddr_in *c_addr);
struct queue *get_queue_client(int idx);

#endif
