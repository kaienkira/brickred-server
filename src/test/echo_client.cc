#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_service.h>

using namespace brickred;

void onNewConnection(TcpService *service,
                     TcpService::SocketId from_socket_id,
                     TcpService::SocketId socket_id)
{
    static int conn_num = 0;
    ::printf("[new connection][%d] %lx from %lx\n",
             ++conn_num, socket_id, from_socket_id);

    // send init message
    static const char message[16 * 1024] = "Hello, world!";
    if (service->sendMessage(socket_id,
                             message, sizeof(message)) == false) {
        service->closeSocket(socket_id);
    }
}

void onRecvMessage(TcpService *service,
                   TcpService::SocketId socket_id,
                   DynamicBuffer *buffer)
{
    std::string buffer_string(buffer->readBegin(), buffer->readableBytes());
    buffer->read(buffer->readableBytes());

    // echo back
    if (service->sendMessage(socket_id,
                             buffer_string.c_str(),
                             buffer_string.size()) == false) {
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

int main(int argc, char *argv[])
{
    if (argc < 4) {
        ::fprintf(stderr, "usage: %s <addr> <port> <conn_num>\n", argv[0]);
        return -1;
    }

    IOService io_service;
    TcpService net_service(io_service);
    net_service.setNewConnectionCallback(
        BRICKRED_BIND_FREE_FUNC(&onNewConnection));
    net_service.setRecvMessageCallback(
        BRICKRED_BIND_FREE_FUNC(&onRecvMessage));
    net_service.setPeerCloseCallback(
        BRICKRED_BIND_FREE_FUNC(&onPeerClose));
    net_service.setErrorCallback(
        BRICKRED_BIND_FREE_FUNC(&onError));

    int conn_num = ::atoi(argv[3]);
    if (conn_num > 60000) {
        conn_num = 60000;
    }

    for (int i = 0; i < conn_num; ++i) {
      bool complete = false;
      if (net_service.asyncConnect(SocketAddress(argv[1], ::atoi(argv[2])),
                                   &complete) < 0) {
          ::fprintf(stderr, "socket async connect failed\n");
          return -1;
      }
    }

    io_service.loop();

    return 0;
}
