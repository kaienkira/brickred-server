#ifndef BRICKRED_PROTOCOL_WEB_SOCKET_PROTOCOL_H
#define BRICKRED_PROTOCOL_WEB_SOCKET_PROTOCOL_H

#include <cstddef>
#include <string>

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred {

class DynamicBuffer;
class Random;
class SocketAddress;

} // namespace brickred

namespace brickred {
namespace protocol {

class WebSocketProtocol {
public:
    struct Status {
        enum type {
            DETACHED,
            WAITING_HANDSHAKE_REQUEST,
            WAITING_HANDSHAKE_RESPONSE,
            CONNECTED,
            FINISHED,
            CLOSED,
            PENDING_ERROR,
            MAX
        };
    };

    struct RetCode {
        enum type {
            ERROR = -1,
            WAITING_MORE_DATA = 0,
            CONNECTION_ESTABLISHED,
            MESSAGE_READY,
            PEER_CLOSED,
            PING_FRAME,
            PONG_FRAME,
        };
    };

    typedef Function<void (const char *, size_t)> OutputCallback;

    WebSocketProtocol();
    ~WebSocketProtocol();

    Status::type getStatus() const;

    void setOutputCallback(const OutputCallback &output_cb);
    void setHandshakeHeader(const std::string &key, const std::string &value);

    // send a handshake to the server
    bool startAsClient(const SocketAddress &peer_addr,
                       Random &random_generator,
                       const char *request_uri = "/");
    // wait a handshake from the client
    bool startAsServer();

    RetCode::type recvMessage(DynamicBuffer *buffer);
    bool retrieveMessage(DynamicBuffer *message);
    void sendMessage(const char *buffer, size_t size);
    void sendCloseFrame();
    void sendPingFrame();

private:
    BRICKRED_NONCOPYABLE(WebSocketProtocol)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace protocol
} // namespace brickred

#endif
