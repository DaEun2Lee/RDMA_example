.PHONY: clean

CFLAGS := -Wall -g -ggdb -Werror
LDLIBS := ${LDLIBS} -lrdmacm -libverbs -lpthread
cc := g++

APP := rdma_main

default: rdma_common.c rdma_server.c rdma_server_handler.c rdma_client.c rdma_client_handler.c rdma_main.c
	${CC} -o ${APP} $^ -DACN_RDMA_DEBUG ${LDLIBS}

clean:
	rm -f ${APP}
