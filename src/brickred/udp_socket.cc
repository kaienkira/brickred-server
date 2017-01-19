#include <brickred/udp_socket.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <cerrno>

namespace brickred {

UdpSocket::UdpSocket()
{
}

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::open(SocketAddress::Protocol::type protocol)
{
    if (fd_ != -1) {
        close();
    }

    if (SocketAddress::Protocol::IP_V4 == protocol) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (-1 == fd_) {
            return false;
        }
    } else if (SocketAddress::Protocol::IP_V6 == protocol) {
        fd_ = ::socket(AF_INET6, SOCK_DGRAM, 0);
        if (-1 == fd_) {
            return false;
        }
    } else {
        errno = EAFNOSUPPORT;
        return false;
    }

    if (setCloseOnExec() == false) {
        close();
        return false;
    }

    return true;
}

void UdpSocket::close()
{
    detachIOService();

    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool UdpSocket::connect(const SocketAddress &addr)
{
    if (addr.getNativeAddress() == NULL) {
        errno = EAFNOSUPPORT;
        return false;
    }

    if (::connect(fd_, (const struct sockaddr *)addr.getNativeAddress(),
                  addr.getNativeAddressSize()) != 0) {
        return false;
    }

    return true;
}

bool UdpSocket::bind(const SocketAddress &addr)
{
    if (addr.getNativeAddress() == NULL) {
        errno = EAFNOSUPPORT;
        return false;
    }

    if (::bind(fd_, (const struct sockaddr *)addr.getNativeAddress(),
               addr.getNativeAddressSize()) != 0) {
        return false;
    }

    return true;
}

int UdpSocket::recv(char *buffer, size_t size)
{
    return ::recv(fd_, buffer, size, 0);
}

int UdpSocket::send(const char *buffer, size_t size)
{
    return ::send(fd_, buffer, size, MSG_NOSIGNAL);
}

int UdpSocket::recvFrom(char *buffer, size_t size, SocketAddress *addr)
{
    struct sockaddr_storage sock_addr;
    socklen_t addr_len = sizeof(sock_addr);

    ssize_t ret = ::recvfrom(fd_, buffer, size, 0,
        (struct sockaddr *)&sock_addr, &addr_len);
    if (ret >= 0) {
        addr->setNativeAddress(&sock_addr);
    }

    return ret;
}

int UdpSocket::sendTo(const char *buffer, size_t size, const SocketAddress &addr)
{
    return ::sendto(fd_, buffer, size, MSG_NOSIGNAL,
                    (const struct sockaddr *)addr.getNativeAddress(),
                    addr.getNativeAddressSize());
}

bool UdpSocket::activeOpen(const SocketAddress &remote_addr)
{
    if (open(remote_addr.getProtocol()) == false) {
        return false;
    }
    if (connect(remote_addr) == false) {
        close();
        return false;
    }

    return true;
}

bool UdpSocket::activeOpenNonblock(const SocketAddress &remote_addr)
{
    if (activeOpen(remote_addr) == false) {
        return false;
    }
    if (setNonblock() == false) {
        close();
        return false;
    }

    return true;
}

bool UdpSocket::passiveOpen(const SocketAddress &local_addr)
{
    if (open(local_addr.getProtocol()) == false) {
        return false;
    }
    if (bind(local_addr) == false) {
        close();
        return false;
    }

    return true;
}

bool UdpSocket::passiveOpenNonblock(const SocketAddress &local_addr)
{
    if (passiveOpen(local_addr) == false) {
        return false;
    }
    if (setNonblock() == false) {
        close();
        return false;
    }

    return true;
}

} // namespace brickred
