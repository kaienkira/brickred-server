#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <ext/hash_set>

#include <brickred/command_line_option.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_service.h>
#include <brickred/thread.h>
#include <brickred/unique_ptr.h>

using namespace brickred;

class AsyncConnector {
public:
    AsyncConnector() :
        tcp_service_(io_service_),
        connect_timer_(-1),
        max_conn_count_(0), conn_count_(0),
        conn_count_once_(0), conn_delay_ms_(0),
        conn_timeout_(0),
        verbose_(false)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &AsyncConnector::onNewConnection, this));
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &AsyncConnector::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &AsyncConnector::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &AsyncConnector::onError, this));
    }

    ~AsyncConnector()
    {
        if (connect_timer_ != -1) {
            io_service_.stopTimer(connect_timer_);
        }
    }

    void init(int id, const SocketAddress &addr, int max_conn_count,
              int conn_count_once, int conn_delay_ms, int conn_timeout,
              bool verbose)
    {
        id_ = id;
        addr_ = addr;
        max_conn_count_ = max_conn_count;
        conn_count_once_ = conn_count_once;
        conn_delay_ms_ = conn_delay_ms;
        conn_timeout_ = conn_timeout;
        verbose_ = verbose;
    }

    void run()
    {
        connect();
        connect_timer_ = io_service_.startTimer(1000,
            BRICKRED_BIND_MEM_FUNC(&AsyncConnector::onTimer, this));
        io_service_.loop();
    }

private:
    void connect()
    {
        if (verbose_) {
            ::printf("thread(%d) connection alive(%d/%d)\n", id_,
                conn_count_ - (int)connecting_sockets_.size(),
                max_conn_count_);
        }

        int count = 0;

        for (int i = conn_count_; i < max_conn_count_; ++i)
        {
            bool complete = false;
            TcpService::SocketId socket_id = tcp_service_.asyncConnect(
                addr_, &complete, conn_timeout_);
            if (socket_id != -1) {
                ++conn_count_;
                connecting_sockets_.insert(socket_id);
            }

            ++count;
            if (count == conn_count_once_) {
                count = 0;
                this_thread::sleepFor(conn_delay_ms_);
            }
        }
    }

    void onTimer(int64_t timer_id)
    {
        connect();
    }

    void onNewConnection(TcpService *service,
                         TcpService::SocketId from_socket_id,
                         TcpService::SocketId socket_id)
    {
        connecting_sockets_.erase(socket_id);
    }

    void onRecvMessage(TcpService *service,
                       TcpService::SocketId socket_id,
                       DynamicBuffer *buffer)
    {
        buffer->read(buffer->readableBytes());
    }

    void onPeerClose(TcpService *service,
                     TcpService::SocketId socket_id)
    {
        service->closeSocket(socket_id);
        --conn_count_;
    }

    void onError(TcpService *service,
                 TcpService::SocketId socket_id,
                 int error)
    {
        service->closeSocket(socket_id);
        connecting_sockets_.erase(socket_id);
        --conn_count_;
    }

private:
    BRICKRED_NONCOPYABLE(AsyncConnector)

    int id_;
    IOService io_service_;
    TcpService tcp_service_;
    SocketAddress addr_;
    int64_t connect_timer_;
    int max_conn_count_;
    int conn_count_;
    int conn_count_once_;
    int conn_delay_ms_;
    int conn_timeout_;
    bool verbose_;

    __gnu_cxx::hash_set<TcpService::SocketId> connecting_sockets_;
};

static void printUsage(const char *progname)
{
    ::fprintf(stderr, "usage: %s <ip> <port>\n"
              "[-c <conn_count_once>]\n"
              "[-d <conn_delay_ms>]\n"
              "[-l <thread_count>]\n"
              "[-n <max_conn_count>]\n"
              "[-t <conn_timeout>]\n",
              progname);
}

int main(int argc, char *argv[])
{
    std::string ip;
    uint16_t port;
    int max_conn_count;
    int conn_count_once = 50;
    int conn_delay_ms = 20;
    int conn_timeout = 5000;
    int thread_count = 1;
    bool verbose = false;

    CommandLineOption options;
    options.addOption("c", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("d", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("l", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("n", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("t", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("v");

    if (options.parse(argc, argv) == false) {
        printUsage(argv[0]);
        return -1;
    }
    if (options.hasOption("c")) {
        conn_count_once = ::atoi(options.getParameter("c").c_str());
    }
    if (options.hasOption("d")) {
        conn_delay_ms = ::atoi(options.getParameter("d").c_str());
    }
    if (options.hasOption("l")) {
        thread_count = ::atoi(options.getParameter("l").c_str());
    }
    if (options.hasOption("n")) {
        max_conn_count = ::atoi(options.getParameter("n").c_str());
    }
    if (options.hasOption("t")) {
        conn_timeout = ::atoi(options.getParameter("t").c_str());
    }
    if (options.hasOption("v")) {
        verbose = true;
    }

    if (options.getLeftArguments().size() != 3) {
        printUsage(argv[0]);
        return -1;
    }
    ip = options.getLeftArguments()[0];
    port = ::atoi(options.getLeftArguments()[1].c_str());
    max_conn_count = ::atoi(options.getLeftArguments()[2].c_str());

    SocketAddress addr(ip, port);
    UniquePtr<Thread[]> threads(new Thread[thread_count]);
    UniquePtr<AsyncConnector[]> connectors(new AsyncConnector[thread_count]);

    for (int i = 0; i < thread_count; ++i) {
        connectors[i].init(i, addr, max_conn_count,
            conn_count_once, conn_delay_ms, conn_timeout, verbose);
    }

    for (int i = 0; i < thread_count; ++i) {
        threads[i].start(BRICKRED_BIND_MEM_FUNC(
            &AsyncConnector::run, &connectors[i]));
    }

    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }

    return 0;
}
