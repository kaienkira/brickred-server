#ifndef BRICKRED_TCP_SERVICE_H
#define BRICKRED_TCP_SERVICE_H

#include <cstddef>
#include <cstdint>

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred { class DynamicBuffer; }
namespace brickred { class IOService; }
namespace brickred { class SocketAddress; }
namespace brickred { class TcpSocket; }

namespace brickred {

class TcpService final {
public:
    class Context {
    public:
        Context() {}
        virtual ~Context() = 0;

    private:
        BRICKRED_NONCOPYABLE(Context)
    };

    using SocketId = int64_t;
    using NewConnectionCallback =
        Function<void (TcpService *, SocketId, SocketId)>;
    using RecvMessageCallback =
        Function<void (TcpService *, SocketId, DynamicBuffer *)>;
    using PeerCloseCallback =
        Function<void (TcpService *, SocketId)>;
    using ErrorCallback =
        Function<void (TcpService *, SocketId, int)>;
    using SendCompleteCallback =
        Function<void (TcpService *, SocketId)>;

    explicit TcpService(IOService &io_service);
    ~TcpService();

    IOService *getIOService() const;

    SocketId listen(const SocketAddress &addr);
    SocketId shareListen(const TcpSocket &shared_socket);
    SocketId connect(const SocketAddress &addr);
    SocketId asyncConnect(const SocketAddress &addr, bool *complete,
                          int timeout_ms = -1);

    bool isConnected(SocketId socket_id) const;

    bool getLocalAddress(SocketId socket_id, SocketAddress *addr) const;
    bool getPeerAddress(SocketId socket_id, SocketAddress *addr) const;

    bool sendMessage(SocketId socket_id, const char *buffer, size_t size,
        const SendCompleteCallback &send_complete_cb = NullFunction());
    bool sendMessageThenClose(SocketId socket_id,
                              const char *buffer, size_t size);
    void broadcastMessage(const char *buffer, size_t size);
    void closeSocket(SocketId socket_id);

    Context *getContext(SocketId socket_id) const;
    bool setContext(SocketId socket_id, Context *context);

    void setNewConnectionCallback(const NewConnectionCallback &new_conn_cb);
    void setRecvMessageCallback(const RecvMessageCallback &recv_message_cb);
    void setPeerCloseCallback(const PeerCloseCallback &peer_close_cb);
    void setErrorCallback(const ErrorCallback &error_cb);

    void setRecvBufferInitSize(size_t size = 1024);
    void setRecvBufferExpandSize(size_t size = 1024);
    void setRecvBufferMaxSize(size_t size = 0);
    void setSendBufferInitSize(size_t size = 1024);
    void setSendBufferExpandSize(size_t size = 1024);
    void setSendBufferMaxSize(size_t size = 0);
    void setAcceptPauseTimeWhenExceedOpenFileLimit(int ms = 0);

private:
    BRICKRED_NONCOPYABLE(TcpService)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred

#endif
