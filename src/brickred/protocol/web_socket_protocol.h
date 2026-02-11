#ifndef BRICKRED_PROTOCOL_WEB_SOCKET_PROTOCOL_H
#define BRICKRED_PROTOCOL_WEB_SOCKET_PROTOCOL_H

#include <cstddef>
#include <string>

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred { class DynamicBuffer; }
namespace brickred { class Random; }
namespace brickred { class SocketAddress; }

namespace brickred::protocol {

class WebSocketProtocol final {
public:
    enum class Status {
        DETACHED,
        WAITING_HANDSHAKE_REQUEST,
        WAITING_HANDSHAKE_RESPONSE,
        CONNECTED,
        FINISHED,
        CLOSED,
        PENDING_ERROR,
        MAX
    };

    enum class RetCode {
        ERROR = -1,
        WAITING_MORE_DATA = 0,
        CONNECTION_ESTABLISHED,
        MESSAGE_READY,
        PEER_CLOSED,
        PING_FRAME,
        PONG_FRAME,
    };

    using OutputCallback = Function<void (const char *, size_t)>;

    WebSocketProtocol();
    ~WebSocketProtocol();

    Status getStatus() const;

    void setOutputCallback(const OutputCallback &output_cb);
    void setHandshakeHeader(const std::string &key, const std::string &value);

    // send a handshake to the server
    bool startAsClient(const SocketAddress &peer_addr,
                       Random &random_generator,
                       const char *request_uri = "/");
    // wait a handshake from the client
    bool startAsServer();

    RetCode recvMessage(DynamicBuffer *buffer);
    bool retrieveMessage(DynamicBuffer *message);
    void sendMessage(const char *buffer, size_t size);
    void sendCloseFrame();
    void sendPingFrame();

    void setMessageMaxSize(size_t size = 1024 * 1024);

private:
    BRICKRED_NONCOPYABLE(WebSocketProtocol)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred::protocol

#endif
