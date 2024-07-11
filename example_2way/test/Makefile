.PHONY: clean

CFLAGS := -Wall -g -ggdb -Werror
LDLIBS := ${LDLIBS} -lrdmacm -libverbs -lpthread
CC := g++

APP := rdma_e

default: rdma_e.c
	${CC} -o ${APP} $^ -DACN_RDMA_DEBUG ${LDLIBS}

clean:
	rm -f ${APP}
