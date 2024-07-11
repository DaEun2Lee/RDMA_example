// rdma_common.h
#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

struct rdma_event_channel *ec;
struct rdma_cm_id *conn;
struct ibv_pd *pd;
struct ibv_comp_channel *comp_chan;
struct ibv_cq *cq;
struct ibv_mr *recv_mr, *send_mr;
struct ibv_qp_init_attr qp_attr;
struct rdma_cm_event *event;
char *recv_region, *send_region;

void* poll_cq(void *arg);
void* receive_thread(void *arg);
void* send_thread(void *arg);

#endif // RDMA_COMMON_H
