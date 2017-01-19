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

class PolicyServer {
public:
    PolicyServer() : tcp_service_(io_service_)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &PolicyServer::onNewConnection, this));
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &PolicyServer::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &PolicyServer::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &PolicyServer::onError, this));
    }

    ~PolicyServer()
    {
    }

    bool run(const SocketAddress &addr)
    {
        if (tcp_service_.listen(addr) < 0) {
            ::fprintf(stderr, "socket listen failed: %s\n",
                      ::strerror(errno));
            return false;
        }

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
        std::string buffer_string(buffer->readBegin(),
                                  buffer->readableBytes());
        buffer->read(buffer->readableBytes());

        if (buffer_string.find("<policy-file-request/>") !=
            std::string::npos) {
            static char policy_file[] =
                "<?xml version=\"1.0\"?>"
                "<cross-domain-policy>"
                    "<allow-access-from domain=\"*\" to-ports=\"*\"/>"
                "</cross-domain-policy>";
            service->sendMessageThenClose(socket_id,
                policy_file, sizeof(policy_file) - 1);
        } else {
            service->closeSocket(socket_id);
        }
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

private:
    IOService io_service_;
    TcpService tcp_service_;
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        ::fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    PolicyServer server;
    if (server.run(SocketAddress(argv[1], ::atoi(argv[2]))) == false) {
        return -1;
    }

    return 0;
}
