#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <brickred/socket_address.h>
#include <brickred/udp_socket.h>

using namespace brickred;

int main(int argc, char *argv[])
{
    if (argc < 5) {
        ::fprintf(stderr, "usage: %s <ip> <port> <message> <count>\n",
                  argv[0]);
        return -1;
    }

    SocketAddress addr(argv[1], ::atoi(argv[2]));
    UdpSocket socket;

    if (socket.activeOpen(addr) == false) {
        ::fprintf(stderr, "socket bind failed: %s\n",
                  ::strerror(errno));
    }

    std::string message = argv[3];
    char buffer[4096];
    int count = ::atoi(argv[4]);

    for (int i = 0; i < count; ++i) {
        socket.send(message.c_str(), message.size());
        int packet_size = socket.recv(buffer, sizeof(buffer) - 1);
        buffer[packet_size] = '\0';
        printf("%s\n", buffer);
    }

    return 0;
}
