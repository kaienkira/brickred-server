#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "test/test_util.h"
#include <brickred/dynamic_buffer.h>
#include <brickred/socket_address.h>
#include <brickred/io_service.h>
#include <brickred/tcp_service.h>
#include <brickred/unique_ptr.h>
#include <brickred/protocol/web_socket_protocol.h>

using namespace brickred;
using namespace brickred::protocol;

class WsEchoServer {
public:
    class Context : public TcpService::Context {
    public:
        Context(TcpService *tcp_service,
                TcpService::SocketId socket_id) :
            tcp_service_(tcp_service),
            socket_id_(socket_id)
        {
        }

        WebSocketProtocol &getProtocol()
        {
            return protocol_;
        }

        void sendMessage(const char *buffer, size_t size)
        {
            tcp_service_->sendMessage(socket_id_, buffer, size);
        }

    private:
        TcpService *tcp_service_;
        TcpService::SocketId socket_id_;
        WebSocketProtocol protocol_;
    };

    WsEchoServer() : tcp_service_(io_service_)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &WsEchoServer::onNewConnection, this));
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &WsEchoServer::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &WsEchoServer::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &WsEchoServer::onError, this));
    }

    ~WsEchoServer()
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

        UniquePtr<Context> context(new Context(service, socket_id));
        context->getProtocol().setOutputCallback(BRICKRED_BIND_MEM_FUNC(
            &WsEchoServer::Context::sendMessage, context.get()));
        context->getProtocol().setHandshakeHeader("Date", "");
        context->getProtocol().startAsServer();

        if (service->setContext(socket_id, context.get()) == false) {
            service->closeSocket(socket_id);
        }
        context.release();
    }

    void onRecvMessage(TcpService *service,
                       TcpService::SocketId socket_id,
                       DynamicBuffer *buffer)
    {

        Context *context = (Context *)service->getContext(socket_id);
        if (NULL == context) {
            service->closeSocket(socket_id);
            return;
        }

        for (;;) {
            WebSocketProtocol::RetCode::type ret =
                context->getProtocol().recvMessage(buffer);
            if (WebSocketProtocol::RetCode::WAITING_MORE_DATA == ret) {
                return;

            } else if (WebSocketProtocol::RetCode::ERROR == ret) {
                ::printf("[error] %lx: recieve message failed\n", socket_id);
                service->closeSocket(socket_id);
                break;

            } else if (WebSocketProtocol::RetCode::PEER_CLOSED == ret) {
                ::printf("[ws peer close] %lx\n", socket_id);
                service->closeSocket(socket_id);
                break;

            } else if (WebSocketProtocol::RetCode::MESSAGE_READY == ret) {
                DynamicBuffer message;
                if (context->getProtocol().retrieveMessage(
                        &message) == false) {
                    ::printf("[error] %lx: retrieve message failed\n",
                             socket_id);
                    service->closeSocket(socket_id);
                    break;
                }
                ::printf("[recieve message] %lx: %zd bytes\n",
                       socket_id, message.readableBytes());
                context->getProtocol().sendMessage(
                    message.readBegin(), message.readableBytes());
            }
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

    WsEchoServer server;
    if (server.run(SocketAddress(argv[1], ::atoi(argv[2]))) == false) {
        return -1;
    }

    return 0;
}
