.PHONY: clean

# Makefile for RDMA Server Program

CC := g++
CFLAGS := -Wall -g -ggdb -Werror
LDLIBS := -lrdmacm -libverbs -lpthread

APP := rdma_server


$(APP): rdma_common.h rdma_server.c rdma_e.cpp
	$(CC) $(CFLAGS) -o $(APP) rdma_server.c -DACN_RDMA_DEBUG $(LDLIBS)
default: $(APP)

clean:
	rm -f $(APP)
