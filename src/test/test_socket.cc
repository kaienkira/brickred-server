#include <iostream>

#include <brickred/condition_variable.h>
#include <brickred/mutex.h>
#include <brickred/socket_address.h>
#include <brickred/thread.h>
#include <brickred/tcp_socket.h>

using namespace brickred;

bool g_server_started = false;
Mutex g_mutex;
ConditionVariable g_server_started_cond;

void server_func()
{
    TcpSocket listen_sock;
    if (listen_sock.passiveOpen(SocketAddress("127.0.0.1", 2000)) == false) {
        std::cerr << "passiveOpen() failed" << std::endl;
        return;
    }

    {
        LockGuard lock(g_mutex);
        g_server_started = true;
        g_server_started_cond.notifyAll();
    }

    for (;;) {
        TcpSocket client_sock;
        if (listen_sock.accept(&client_sock) == false) {
            std::cerr << "accept() failed" << std::endl;
            return;
        }
        char buff[1024];
        client_sock.recv(buff, sizeof(buff));
        std::cout << buff << std::endl;
    }
}

void client_func()
{
    {
        LockGuard lock(g_mutex);
        while (!g_server_started) {
            g_server_started_cond.wait(g_mutex);
        }
    }

    for (size_t i = 0; i < 10; ++i) {
        TcpSocket sock;
        if (sock.activeOpen(SocketAddress("127.0.0.1", 2000)) == false) {
            std::cerr << "activeOpen() failed" << std::endl;
            return;
        }
        sock.send("hello, world!", 14);
    }
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
