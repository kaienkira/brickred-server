#ifndef BRICKRED_UDP_SOCKET_H
#define BRICKRED_UDP_SOCKET_H

#include <cstddef>

#include <brickred/class_util.h>
#include <brickred/io_device.h>
#include <brickred/socket_address.h>

namespace brickred {

class UdpSocket : public IODevice {
public:
    UdpSocket();
    virtual ~UdpSocket();

    bool open(SocketAddress::Protocol::type protocol);
    void close();
    bool connect(const SocketAddress &addr);
    bool bind(const SocketAddress &addr);

    int recv(char *buffer, size_t size);
    int send(const char *buffer, size_t size);
    int recvFrom(char *buffer, size_t size, SocketAddress *addr);
    int sendTo(const char *buffer, size_t size, const SocketAddress &addr);

    // -*- udp options -*-
    int getSocketError();

    // -*- builder methods -*-
    // open()
    // connect()
    bool activeOpen(const SocketAddress &remote_addr);
    // activeOpen()
    // setNonblock()
    bool activeOpenNonblock(const SocketAddress &remote_addr);
    // open()
    // bind()
    bool passiveOpen(const SocketAddress &local_addr);
    // passiveOpen()
    // setNonblock()
    bool passiveOpenNonblock(const SocketAddress &local_addr);

private:
    BRICKRED_NONCOPYABLE(UdpSocket)
};

} // namespace brickred

#endif
