#include <brickred/protocol/web_socket_protocol.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

#include <brickred/dynamic_buffer.h>
#include <brickred/random.h>
#include <brickred/socket_address.h>
#include <brickred/string_util.h>
#include <brickred/codec/base64.h>
#include <brickred/codec/sha1.h>
#include <brickred/protocol/http_message.h>
#include <brickred/protocol/http_protocol.h>
#include <brickred/protocol/http_request.h>
#include <brickred/protocol/http_response.h>

namespace brickred::protocol {

class WebSocketProtocol::Impl {
public:
    using Status = WebSocketProtocol::Status;
    using RetCode = WebSocketProtocol::RetCode;
    using OutputCallback = WebSocketProtocol::OutputCallback;
    using HeaderMap = std::map<std::string, std::string,
                               string_util::CaseInsensitiveLess>;
    using StatusHandler = int (WebSocketProtocol::Impl::*)(DynamicBuffer *);

    Impl();
    ~Impl();

    Status::type getStatus() const { return status_; }

    void setOutputCallback(const OutputCallback &output_cb);
    void setHandshakeHeader(const std::string &key, const std::string &value);

    bool startAsClient(const SocketAddress &peer_addr,
                       Random &random_generator,
                       const char *request_uri);
    bool startAsServer();

    RetCode::type recvMessage(DynamicBuffer *buffer);
    bool retrieveMessage(DynamicBuffer *message);
    void sendMessage(const char *buffer, size_t size);
    void sendCloseFrame();
    void sendPingFrame();

public:
    bool checkHandshakeRequestValid(const HttpRequest &request) const;
    void sendHandshakeErrorResponse();
    void sendHandshakeSuccessResponse(const HttpRequest &request);
    bool checkHandshakeResponseValid(
        const HttpResponse &response, const std::string &sec_key) const;

    int readHandshakeRequest(DynamicBuffer *buffer);
    int readHandshakeResponse(DynamicBuffer *buffer);
    int readFrame(DynamicBuffer *buffer);
    void sendPongFrame();

private:
    static StatusHandler s_status_handler_[Status::MAX];

private:
    Status::type status_;
    OutputCallback output_cb_;
    Random *random_generator_;
    HttpProtocol http_protocol_;
    HeaderMap handshake_headers_;
    DynamicBuffer message_;
    bool is_client_;
    bool close_frame_sent_;
    int last_op_code_;
    RetCode::type ret_code_;
};

///////////////////////////////////////////////////////////////////////////////
WebSocketProtocol::Impl::StatusHandler
WebSocketProtocol::Impl::s_status_handler_[] = {
    nullptr,
    &WebSocketProtocol::Impl::readHandshakeRequest,
    &WebSocketProtocol::Impl::readHandshakeResponse,
    &WebSocketProtocol::Impl::readFrame,
    nullptr,
    nullptr,
    nullptr
};

WebSocketProtocol::Impl::Impl() :
    status_(Status::DETACHED),
    random_generator_(nullptr),
    is_client_(false), close_frame_sent_(false), last_op_code_(-1),
    ret_code_(RetCode::ERROR)
{
}

WebSocketProtocol::Impl::~Impl()
{
}

void WebSocketProtocol::Impl::setOutputCallback(
    const OutputCallback &output_cb)
{
    http_protocol_.setOutputCallback(output_cb);
    output_cb_ = output_cb;
}

void WebSocketProtocol::Impl::setHandshakeHeader(const std::string &key,
                                                 const std::string &value)
{
    handshake_headers_[key] = value;
}

bool WebSocketProtocol::Impl::startAsClient(
    const SocketAddress &peer_addr,
    Random &random_generator, const char *request_uri)
{
    if (status_ != Status::DETACHED) {
        return false;
    }

    status_ = Status::WAITING_HANDSHAKE_RESPONSE;
    random_generator_ = &random_generator;
    is_client_ = true;

    // send handshake
    HttpRequest request;
    request.setVersion(HttpMessage::Version::HTTP_1_1);
    request.setMethod(HttpRequest::Method::GET);
    request.setRequestUri(request_uri);
    request.setHeader("Upgrade", "websocket");
    request.setHeader("Connection", "Upgrade");
    request.setHeader("Sec-WebSocket-Version", "13");

    // set header 'Sec-WebSocket-Key' if not provided
    if (handshake_headers_.find("Sec-WebSocket-Key") ==
        handshake_headers_.end()) {
        std::vector<char> key(16);
        for (size_t i = 0; i < key.size(); ++i) {
            key[i] = random_generator_->nextInt(256);
        }
        setHandshakeHeader("Sec-WebSocket-Key",
                           codec::base64Encode(&key[0], key.size()));
    }
    // set header 'Host' if not provided
    if (handshake_headers_.find("Host") ==
        handshake_headers_.end()) {
        setHandshakeHeader("Host",
            peer_addr.getIp() + ":" +
            string_util::toString(peer_addr.getPort()));
    }

    // set handshake headers
    for (HeaderMap::const_iterator iter = handshake_headers_.begin();
         iter != handshake_headers_.end(); ++iter) {
        request.setHeader(iter->first, iter->second);
    }

    http_protocol_.sendMessage(request);

    return true;
}

bool WebSocketProtocol::Impl::startAsServer()
{
    if (status_ != Status::DETACHED) {
        return false;
    }

    status_ = Status::WAITING_HANDSHAKE_REQUEST;
    is_client_ = false;

    return true;
}

WebSocketProtocol::RetCode::type WebSocketProtocol::Impl::recvMessage(
    DynamicBuffer *buffer)
{
    for (;;) {
        StatusHandler func = s_status_handler_[status_];
        if (nullptr == func) {
            return RetCode::ERROR;
        }

        int ret = (this->*func)(buffer);
        if (0 == ret) {
            // not finished, wait for more data
            return RetCode::WAITING_MORE_DATA;

        } else if (-1 == ret) {
            // error occured
            status_ = Status::PENDING_ERROR;
            return RetCode::ERROR;

        } else if (2 == ret) {
            // special event
            return ret_code_;
        }

        if (Status::FINISHED == status_) {
            return RetCode::MESSAGE_READY;
        } else if (Status::CLOSED == status_) {
            return RetCode::PEER_CLOSED;
        }
    }
}

bool WebSocketProtocol::Impl::checkHandshakeRequestValid(
    const HttpRequest &request) const
{
    // check header 'Host'
    if (request.hasHeader("Host") == false) {
        return false;
    }
    // check header 'Upgrade'
    if (request.headerContain("Upgrade", "websocket") == false) {
        return false;
    }
    // check header 'Connection'
    if (request.headerContain("Connection", "Upgrade") == false) {
        return false;
    }
    // check header 'Sec-WebSocket-Key'
    const std::string &sec_key = request.getHeader("Sec-WebSocket-Key");
    if (sec_key.size() != 24) {
        return false;
    }
    // check header 'Sec-WebSocket-Version'
    if (request.headerEqual("Sec-WebSocket-Version", "13") == false) {
        return false;
    }

    return true;
}

void WebSocketProtocol::Impl::sendHandshakeErrorResponse()
{
    static const char http_400[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
    if (output_cb_) {
        output_cb_(http_400, sizeof(http_400) - 1);
    }
}

void WebSocketProtocol::Impl::sendHandshakeSuccessResponse(
    const HttpRequest &request)
{
    const std::string &sec_key = request.getHeader("Sec-WebSocket-Key");

    HttpResponse response;
    response.setVersion(HttpMessage::Version::HTTP_1_1);
    response.setStatusCode(101);
    response.setReasonPhrase("Switching Protocols");
    response.setHeader("Upgrade", "websocket");
    response.setHeader("Connection", "Upgrade");
    response.setHeader("Sec-WebSocket-Accept",
        codec::base64Encode(codec::sha1Binary(sec_key +
            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")));

    // set handshake headers
    for (HeaderMap::const_iterator iter = handshake_headers_.begin();
         iter != handshake_headers_.end(); ++iter) {
        response.setHeader(iter->first, iter->second);
    }

    // process header 'date'
    if (response.hasHeader("Date")) {
        response.setDate();
    }

    http_protocol_.sendMessage(response);
}

int WebSocketProtocol::Impl::readHandshakeRequest(DynamicBuffer *buffer)
{
    HttpProtocol::RetCode::type ret = http_protocol_.recvMessage(buffer);
    if (HttpProtocol::RetCode::WAITING_MORE_DATA == ret) {
        return 0;

    } else if (HttpProtocol::RetCode::MESSAGE_READY == ret) {
        HttpRequest request;
        if (http_protocol_.retrieveRequest(&request) == false) {
            sendHandshakeErrorResponse();
            return -1;
        }

        if (checkHandshakeRequestValid(request) == false) {
            sendHandshakeErrorResponse();
            return -1;
        }

        sendHandshakeSuccessResponse(request);

        status_ = Status::CONNECTED;
        ret_code_ = RetCode::CONNECTION_ESTABLISHED;
        return 2;

    } else {
        sendHandshakeErrorResponse();
        return -1;
    }
}

bool WebSocketProtocol::Impl::checkHandshakeResponseValid(
    const HttpResponse &response, const std::string &sec_key) const
{
    // check status code
    if (response.getStatusCode() != 101) {
        return false;
    }

    // check header 'Upgrade'
    if (response.headerContain("Upgrade", "websocket") == false) {
        return false;
    }

    // check header 'Connection'
    if (response.headerContain("Connection", "Upgrade") == false) {
        return false;
    }

    // check header 'Sec-WebSocket-Accept'
    std::string sec_accept = codec::base64Encode(codec::sha1Binary(sec_key +
        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
    if (response.headerEqual("Sec-WebSocket-Accept", sec_accept) == false) {
        return false;
    }

    return true;
}

int WebSocketProtocol::Impl::readHandshakeResponse(DynamicBuffer *buffer)
{
    HttpProtocol::RetCode::type ret = http_protocol_.recvMessage(buffer);
    if (HttpProtocol::RetCode::WAITING_MORE_DATA == ret) {
        return 0;

    } else if (HttpProtocol::RetCode::MESSAGE_READY == ret) {
        HttpResponse response;
        if (http_protocol_.retrieveResponse(&response) == false) {
            return -1;
        }

        if (checkHandshakeResponseValid(response,
                handshake_headers_["Sec-WebSocket-Key"]) == false) {
            return -1;
        }

        status_ = Status::CONNECTED;
        ret_code_ = RetCode::CONNECTION_ESTABLISHED;
        return 2;

    } else {
        return -1;
    }
}

int WebSocketProtocol::Impl::readFrame(DynamicBuffer *buffer)
{
    if (buffer->readableBytes() < 2) {
        return 0;
    }

    const uint8_t *b = (uint8_t *)buffer->readBegin();

    // check RSV1, RSV2, RSV3
    if ((b[0] & 0x70) != 0) {
        return -1;
    }

    const uint8_t *p = b + 2;
    size_t left_bytes = buffer->readableBytes() - 2;

    // get payload length
    uint64_t payload_length = b[1] & 0x7f;
    if (126 == payload_length) {
        // get uint16_t length
        if (buffer->peekInt16(payload_length, p - b) == false) {
            return 0;
        }

        // the minimal number of bytes MUST be used to encode the length
        if (payload_length < 126) {
            return -1;
        }

        // move forward
        p += 2;
        left_bytes -= 2;

    } else if (127 == payload_length) {
        // get uint64_t length
        if (buffer->peekInt64(payload_length, p - b) == false) {
            return 0;
        }

        // the minimal number of bytes MUST be used to encode the length
        if (payload_length < 0xffff) {
            return -1;
        }

        // move forward
        p += 8;
        left_bytes -= 8;
    }

    // get mask key
    bool mask = b[1] & 0x80;
    const uint8_t *mask_key = nullptr;
    if (mask) {
        // A server MUST NOT mask any frames that it sends to the client
        if (is_client_) {
            return -1;
        }
        // check enough data
        if (left_bytes < 4) {
            return 0;
        }

        mask_key = p;

        // move forward
        p += 4;
        left_bytes -= 4;
    }

    // get payload
    if (payload_length > 0) {
        // check enough data for payload
        if (left_bytes < payload_length) {
            return 0;
        }
        // copy payload
        message_.reserveWritableBytes(payload_length);
        ::memcpy(message_.writeBegin(), p, payload_length);
        // do masking
        if (mask) {
            for (size_t i = 0; i < payload_length; ++i) {
                *(message_.writeBegin() + i) ^= mask_key[i & 0x03];
            }
        }
        message_.write(payload_length);
        // move forward
        p += payload_length;
        left_bytes -= payload_length;
    }

    buffer->read(p - b);

    // process frame
    bool fin = b[0] & 0x80;
    int opcode = b[0] & 0x0f;

    // not fin
    if (!fin) {
        if (opcode != 0x0) {
            last_op_code_ = opcode;
        }
        return 0;
    }

    // fin
    if (0x0 == opcode) {
        opcode = last_op_code_;
        last_op_code_ = -1;
    }

    if (0x1 == opcode || 0x2 == opcode) {
        // data frame
        status_ = Status::FINISHED;
        return 1;

    } else if (0x8 == opcode) {
        // close frame
        sendCloseFrame();
        status_ = Status::CLOSED;
        message_.clear();
        return 1;

    } else if (0x9 == opcode) {
        // ping frame
        message_.clear();
        sendPongFrame();
        ret_code_ = RetCode::PING_FRAME;
        return 2;

    } else if (0xa == opcode) {
        // pong frame
        message_.clear();
        ret_code_ = RetCode::PONG_FRAME;
        return 2;

    } else {
        return -1;
    }
}

bool WebSocketProtocol::Impl::retrieveMessage(DynamicBuffer *message)
{
    if (status_ != Status::FINISHED) {
        return false;
    }

    message->swap(message_);
    message_.clear();

    status_ = Status::CONNECTED;

    return true;
}

void WebSocketProtocol::Impl::sendMessage(const char *buffer, size_t size)
{
    if (status_ != Status::CONNECTED) {
        return;
    }
    if (close_frame_sent_) {
        return;
    }

    DynamicBuffer message;

    message.reserveWritableBytes(2);
    // FIN = 1, RSV1~RSV3 = 0, opcode = 0x2
    message.writeBegin()[0] = 0x82;
    message.write(1);

    // mask flag
    message.writeBegin()[0] = (is_client_) ? 0x80 : 0x0;

    // payload length
    if (size < 126) {
        message.writeBegin()[0] |= size;
        message.write(1);
    } else if (size < 65535) {
        message.writeBegin()[0] |= 126;
        message.write(1);
        message.writeInt16(size);
    } else {
        message.writeBegin()[0] |= 127;
        message.write(1);
        message.writeInt64(size);
    }

    // mask key
    const uint8_t *mask_key = nullptr;
    if (is_client_) {
        message.reserveWritableBytes(4);
        for (size_t i = 0; i < 4; ++i) {
            message.writeBegin()[i] = random_generator_->nextInt(256);
        }
        mask_key = (uint8_t *)message.writeBegin();
        message.write(4);
    }

    // payload
    message.reserveWritableBytes(size);
    ::memcpy(message.writeBegin(), buffer, size);
    // do masking
    if (mask_key != nullptr) {
        for (size_t i = 0; i < size; ++i) {
            message.writeBegin()[i] ^= mask_key[i & 0x03];
        }
    }
    message.write(size);

    if (output_cb_) {
        output_cb_(message.readBegin(), message.readableBytes());
    }
}

void WebSocketProtocol::Impl::sendCloseFrame()
{
    // FIN = 1, RSV1~RSV3 = 0, opcode = 0x8, payload_length = 0
    static const uint8_t client_frame[] = { 0x88, 0x80, 0x0, 0x0, 0x0, 0x0};
    static const uint8_t server_frame[] = { 0x88, 0x0 };

    if (status_ != Status::CONNECTED) {
        return;
    }
    if (close_frame_sent_) {
        return;
    }

    close_frame_sent_ = true;

    if (output_cb_) {
        if (is_client_) {
            output_cb_((const char *)client_frame, sizeof(client_frame));
        } else {
            output_cb_((const char *)server_frame, sizeof(server_frame));
        }
    }
}

void WebSocketProtocol::Impl::sendPingFrame()
{
    // FIN = 1, RSV1~RSV3 = 0, opcode = 0x9, payload_length = 0
    static const uint8_t client_frame[] = { 0x89, 0x80, 0x0, 0x0, 0x0, 0x0};
    static const uint8_t server_frame[] = { 0x89, 0x0 };

    if (status_ != Status::CONNECTED) {
        return;
    }
    if (close_frame_sent_) {
        return;
    }

    if (output_cb_) {
        if (is_client_) {
            output_cb_((const char *)client_frame, sizeof(client_frame));
        } else {
            output_cb_((const char *)server_frame, sizeof(server_frame));
        }
    }
}

void WebSocketProtocol::Impl::sendPongFrame()
{
    // FIN = 1, RSV1~RSV3 = 0, opcode = 0xa, payload_length = 0
    static const uint8_t client_frame[] = { 0x8a, 0x80, 0x0, 0x0, 0x0, 0x0};
    static const uint8_t server_frame[] = { 0x8a, 0x0 };

    if (status_ != Status::CONNECTED) {
        return;
    }
    if (close_frame_sent_) {
        return;
    }

    if (output_cb_) {
        if (is_client_) {
            output_cb_((const char *)client_frame, sizeof(client_frame));
        } else {
            output_cb_((const char *)server_frame, sizeof(server_frame));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
WebSocketProtocol::WebSocketProtocol() :
    pimpl_(new Impl())
{
}

WebSocketProtocol::~WebSocketProtocol()
{
}

WebSocketProtocol::Status::type WebSocketProtocol::getStatus() const
{
    return pimpl_->getStatus();
}

void WebSocketProtocol::setOutputCallback(const OutputCallback &output_cb)
{
    pimpl_->setOutputCallback(output_cb);
}

void WebSocketProtocol::setHandshakeHeader(const std::string &key,
                                           const std::string &value)
{
    pimpl_->setHandshakeHeader(key, value);
}

bool WebSocketProtocol::startAsClient(
    const SocketAddress &peer_addr, Random &random_generator,
    const char *request_uri)
{
    return pimpl_->startAsClient(peer_addr, random_generator, request_uri);
}

bool WebSocketProtocol::startAsServer()
{
    return pimpl_->startAsServer();
}

WebSocketProtocol::RetCode::type WebSocketProtocol::recvMessage(
    DynamicBuffer *buffer)
{
    return pimpl_->recvMessage(buffer);
}

bool WebSocketProtocol::retrieveMessage(DynamicBuffer *message)
{
    return pimpl_->retrieveMessage(message);
}

void WebSocketProtocol::sendMessage(const char *buffer, size_t size)
{
    pimpl_->sendMessage(buffer, size);
}

void WebSocketProtocol::sendCloseFrame()
{
    pimpl_->sendCloseFrame();
}

void WebSocketProtocol::sendPingFrame()
{
    pimpl_->sendPingFrame();
}

} // namespace brickred::protocol
