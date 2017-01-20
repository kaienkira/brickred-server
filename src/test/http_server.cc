#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "test/test_util.h"
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_service.h>
#include <brickred/unique_ptr.h>
#include <brickred/protocol/http_protocol.h>
#include <brickred/protocol/http_request.h>

using namespace brickred;
using namespace brickred::protocol;

static char s_http_200[] = "HTTP/1.1 200 OK\r\nContent-length: 0\r\n\r\n";
static char s_http_400[] = "HTTP/1.1 400 Bad Request\r\n\r\n";

class HttpServer {
public:
    class Context : public TcpService::Context {
    public:
        HttpProtocol protocol_;
    };

    HttpServer() : tcp_service_(io_service_)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpServer::onNewConnection, this));
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpServer::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpServer::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpServer::onError, this));
    }

    ~HttpServer()
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

        UniquePtr<Context> context(new Context());
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
            HttpProtocol::RetCode::type ret =
                context->protocol_.recvMessage(buffer);
            if (HttpProtocol::RetCode::WAITING_MORE_DATA == ret) {
                return;

            } else if (HttpProtocol::RetCode::MESSAGE_READY == ret) {
                HttpRequest request;
                if (context->protocol_.retrieveRequest(&request) == false) {
                    ::printf("[error] %lx: retrieve request failed\n",
                             socket_id);
                    tcp_service_.sendMessageThenClose(socket_id,
                        s_http_400, sizeof(s_http_400) - 1);
                    return;
                }

                ::printf("[recv http request] %lx: %s\n",
                         socket_id, request.getRequestUri().c_str());
                printHttpRequest(request);

                if (request.isConnectionKeepAlive()) {
                    tcp_service_.sendMessage(socket_id,
                        s_http_200, sizeof(s_http_200) - 1);
                } else {
                    tcp_service_.sendMessageThenClose(socket_id,
                        s_http_200, sizeof(s_http_200) - 1);
                    return;
                }

            } else {
                ::printf("[error] %lx: recieve message failed\n", socket_id);
                tcp_service_.sendMessageThenClose(socket_id,
                    s_http_400, sizeof(s_http_400) - 1);
                return;
            }
        }
    }

    void printHttpRequest(const HttpRequest &request)
    {
        // print header
        ::printf("  header:\n");
        for (HttpMessage::HeaderMap::const_iterator iter =
                 request.getHeaders().begin();
             iter != request.getHeaders().end(); ++iter) {
            ::printf("    %s: %s\n",
                     iter->first.c_str(), iter->second.c_str());
        }
        // print body
        ::printf("  body:\n");
        test::hexdump(request.getBody().c_str(), request.getBody().size());
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

    HttpServer server;
    if (server.run(SocketAddress(argv[1], ::atoi(argv[2]))) == false) {
        return -1;
    }

    return 0;
}
