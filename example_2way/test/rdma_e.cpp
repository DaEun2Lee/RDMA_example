#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <infiniband/verbs.h>

#define RDMA_SEND_RECV_BUF_SIZE (1024 * 1024)

struct rdma_context {
    struct ibv_context *context;
    struct ibv_pd *pd;
    struct ibv_cq *send_cq;
    struct ibv_cq *recv_cq;
    struct ibv_qp *qp;
    struct ibv_mr *mr;
    char *buf;
    uint64_t buf_addr;
    size_t buf_size;
};

struct rdma_context ctx;

std::mutex mtx;
std::condition_variable cv;
bool ready_to_receive = false;

void die(const char *reason) {
    std::cerr << "Error: " << reason << std::endl;
    exit(EXIT_FAILURE);
}

void rdma_setup(const char *ip, int port, bool is_server) {
    // Initialize RDMA context
    memset(&ctx, 0, sizeof(ctx));

    // Open device and allocate resources
    struct ibv_device **dev_list;
    dev_list = ibv_get_device_list(nullptr);
    if (!dev_list)
        die("Failed to get IB devices list");

    ctx.context = ibv_open_device(dev_list[0]);  // Assuming you want to open the first device in the list
    if (!ctx.context)
        die("Failed to open device");

    ibv_free_device_list(dev_list);  // Free the device list after use

    ctx.pd = ibv_alloc_pd(ctx.context);
    if (!ctx.pd)
        die("Failed to allocate PD");

    // Create completion queues
    ctx.send_cq = ibv_create_cq(ctx.context, 1, nullptr, nullptr, 0);
    if (!ctx.send_cq)
        die("Failed to create send CQ");

    ctx.recv_cq = ibv_create_cq(ctx.context, 1, nullptr, nullptr, 0);
    if (!ctx.recv_cq)
        die("Failed to create receive CQ");

    // Create QP (Queue Pair)
    struct ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.send_cq = ctx.send_cq;
    qp_init_attr.recv_cq = ctx.recv_cq;
    qp_init_attr.cap.max_send_wr = 1;  // Max outstanding send requests
    qp_init_attr.cap.max_recv_wr = 1;  // Max outstanding receive requests
    qp_init_attr.cap.max_send_sge = 1; // Max send scatter/gather elements
    qp_init_attr.cap.max_recv_sge = 1; // Max receive scatter/gather elements

    ctx.qp = ibv_create_qp(ctx.pd, &qp_init_attr);
    if (!ctx.qp)
        die("Failed to create QP");

    // Allocate memory buffer and register it with HCA (RDMA device)
    ctx.buf = static_cast<char *>(malloc(RDMA_SEND_RECV_BUF_SIZE));
    if (!ctx.buf)
        die("Failed to allocate memory buffer");

    memset(ctx.buf, 0, RDMA_SEND_RECV_BUF_SIZE);

    ctx.mr = ibv_reg_mr(ctx.pd, ctx.buf, RDMA_SEND_RECV_BUF_SIZE, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    if (!ctx.mr)
        die("Failed to register MR");

    ctx.buf_addr = reinterpret_cast<uint64_t>(ctx.buf);
    ctx.buf_size = RDMA_SEND_RECV_BUF_SIZE;

    // Transition QP to the RTS (Ready to Send) state
    struct ibv_qp_attr qp_attr = {};
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = 1;  // Assuming port 1

    if (ibv_modify_qp(ctx.qp, &qp_attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT))
        die("Failed to transition QP to INIT state");

    qp_attr.qp_state = IBV_QPS_RTR;  // Ready to Receive
    if (ibv_modify_qp(ctx.qp, &qp_attr, IBV_QP_STATE))
        die("Failed to transition QP to RTR state");

    qp_attr.qp_state = IBV_QPS_RTS;  // Ready to Send
    if (ibv_modify_qp(ctx.qp, &qp_attr, IBV_QP_STATE))
        die("Failed to transition QP to RTS state");

    if (is_server) {
        // Server-specific setup
        // Start a separate thread to handle incoming connections and data
        std::thread server_thread([](){
            char recv_buf[RDMA_SEND_RECV_BUF_SIZE];

            while (true) {
                rdma_receive(recv_buf, RDMA_SEND_RECV_BUF_SIZE);
                std::cout << "Received data: " << recv_buf << std::endl;

                // Signal sender that data is received
                std::unique_lock<std::mutex> lock(mtx);
                ready_to_receive = true;
                cv.notify_one();
            }
        });

        server_thread.detach(); // Detach the thread so it runs independently
    } else {
        // Client-specific setup
        // Start a separate thread to send data
        std::thread client_thread([](){
            const char *data_to_send = "Hello, RDMA!";
            while (true) {
                rdma_send(data_to_send, strlen(data_to_send) + 1);  // Include null terminator
                usleep(1000000);  // Wait 1 second between sends
            }
        });

        client_thread.detach(); // Detach the thread so it runs independently
    }
}

void rdma_send(const char *data, size_t size) {
    struct ibv_send_wr send_wr = {};
    struct ibv_sge sge = {};

    sge.addr = reinterpret_cast<uint64_t>(const_cast<char *>(data));
    sge.length = size;
    sge.lkey = ctx.mr->lkey;

    send_wr.sg_list = &sge;
    send_wr.num_sge = 1;
    send_wr.opcode = IBV_WR_SEND;
    send_wr.send_flags = IBV_SEND_SIGNALED;

    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(ctx.qp, &send_wr, &bad_wr))
        die("Failed to post send");

    std::cout << "Sent data of size " << size << " bytes" << std::endl;
}

void rdma_receive(char *recv_buf, size_t buf_size) {
    struct ibv_recv_wr recv_wr = {};
    struct ibv_sge sge = {};

    sge.addr = reinterpret_cast<uint64_t>(recv_buf);
    sge.length = buf_size;
    sge.lkey = ctx.mr->lkey;

    recv_wr.sg_list = &sge;
    recv_wr.num_sge = 1;

    struct ibv_recv_wr *bad_wr;
    if (ibv_post_recv(ctx.qp, &recv_wr, &bad_wr))
        die("Failed to post receive");

    std::unique_lock<std::mutex> lock(mtx);
    ready_to_receive = false;  // Set to false before waiting
    cv.wait(lock, []{ return ready_to_receive; });
    ready_to_receive = false;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string ip = argv[1];
    int port = std::atoi(argv[2]);

    rdma_setup(ip.c_str(), port, true);  // Start as server

    // Start a client thread to send data
    std::thread client_thread([](const std::string &ip, int port){
        rdma_setup(ip.c_str(), port, false);  // Start as client
    }, ip, port);

    client_thread.join(); // Wait for the client thread to finish

    // Clean up (this part is never reached in the infinite loop)
    ibv_dereg_mr(ctx.mr);
    ibv_destroy_qp(ctx.qp);
    ibv_destroy_cq(ctx.send_cq);
    ibv_destroy_cq(ctx.recv_cq);
    ibv_dealloc_pd(ctx.pd);
    ibv_close_device(ctx.context);

    free(ctx.buf);

    return 0;
}
