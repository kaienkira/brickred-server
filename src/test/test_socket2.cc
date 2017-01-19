#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <brickred/condition_variable.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/mutex.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_service.h>
#include <brickred/thread.h>

using namespace brickred;

bool g_server_started = false;
Mutex g_mutex;
ConditionVariable g_server_started_cond;

void onNewConnection(TcpService *service,
                     TcpService::SocketId from_socket_id,
                     TcpService::SocketId socket_id)
{
    ::printf("[new connection] %lx from %lx\n", socket_id, from_socket_id);
}

void onRecvMessage(TcpService *service,
                   TcpService::SocketId socket_id,
                   DynamicBuffer *buffer)
{
    std::string buffer_string(buffer->readBegin(), buffer->readableBytes());
    buffer->read(buffer->readableBytes());
    ::printf("[receive data] %lx: %s\n", socket_id, buffer_string.c_str());
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
}

void server_func()
{
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
    if (net_service.listen(SocketAddress("127.0.0.1", 2000)) < 0) {
        ::fprintf(stderr, "socket listen failed\n");
        exit(-1);
    }

    {
        LockGuard lock(g_mutex);
        g_server_started = true;
        g_server_started_cond.notifyAll();
    }

    io_service.loop();
}

void client_func()
{
    {
        LockGuard lock(g_mutex);
        while (!g_server_started) {
            g_server_started_cond.wait(g_mutex);
        }
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

    bool complete = false;
    if (net_service.asyncConnect(SocketAddress("127.0.0.1", 2000),
                                 &complete) < 0) {
        ::fprintf(stderr, "socket async connect failed\n");
        exit(-1);
    }

    io_service.loop();
}

int main(void)
{
    Thread server;
    Thread client;

    server.start(BRICKRED_BIND_FREE_FUNC(&server_func));
    client.start(BRICKRED_BIND_FREE_FUNC(&client_func));
    server.join();
    client.join();

    return 0;
}
