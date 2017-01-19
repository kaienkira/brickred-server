#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <brickred/socket_address.h>
#include <brickred/udp_socket.h>

using namespace brickred;

int main(int argc, char *argv[])
{
    if (argc < 3) {
        ::fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    SocketAddress addr(argv[1], ::atoi(argv[2]));
    UdpSocket socket;

    if (socket.passiveOpen(addr) == false) {
        ::fprintf(stderr, "socket bind failed: %s\n",
                  ::strerror(errno));
    }

    for (;;) {
        SocketAddress peer_addr;
        char buffer[4096];
        int recv_bytes = socket.recvFrom(buffer, sizeof(buffer), &peer_addr);
        if (recv_bytes > 0) {
            socket.sendTo(buffer, recv_bytes, peer_addr);
        }
    }

    return 0;
}
