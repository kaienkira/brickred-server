#include <stdint.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_service.h>

using namespace brickred;

class BroadcastServer {
public:
    BroadcastServer() : tcp_service_(io_service_)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &BroadcastServer::onNewConnection, this));
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &BroadcastServer::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &BroadcastServer::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &BroadcastServer::onError, this));
    }

    ~BroadcastServer()
    {
    }

    bool run(const SocketAddress &addr)
    {
        if (tcp_service_.listen(addr) < 0) {
            ::fprintf(stderr, "socket listen failed: %s\n",
                      ::strerror(errno));
            return false;
        }

        timer_id_ = io_service_.startTimer(1000,
            BRICKRED_BIND_MEM_FUNC(&BroadcastServer::onTimer, this));

        io_service_.loop();

        return true;
    }

    void onNewConnection(TcpService *service,
                         TcpService::SocketId from_socket_id,
                         TcpService::SocketId socket_id)
    {
        static int conn_num = 0;
        ::printf("[new connection][%d] %lx from %lx\n",
                 ++conn_num, socket_id, from_socket_id);
    }

    void onRecvMessage(TcpService *service,
                       TcpService::SocketId socket_id,
                       DynamicBuffer *buffer)
    {
        // discard data
        buffer->read(buffer->readableBytes());
    }

    void onPeerClose(TcpService *service,
                     TcpService::SocketId socket_id)
    {
        ::printf("[peer close] %lx\n", socket_id);
        service->closeSocket(socket_id);
    }

    void onError(TcpService *service,
                 TcpService::SocketId socket_id,
                 int error)
    {
        ::printf("[error] %lx: %s\n", socket_id, ::strerror(error));
        service->closeSocket(socket_id);
    }

    void onTimer(int64_t timer_id)
    {
        tcp_service_.broadcastMessage("Hello, world!", 13);
    }

private:
    IOService io_service_;
    TcpService tcp_service_;
    int64_t timer_id_;
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        ::fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    BroadcastServer server;
    if (server.run(SocketAddress(argv[1], ::atoi(argv[2]))) == false) {
        return -1;
    }

    return 0;
}
