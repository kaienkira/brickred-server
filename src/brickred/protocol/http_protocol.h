#ifndef BRICKRED_PROTOCOL_HTTP_PROTOCOL_H
#define BRICKRED_PROTOCOL_HTTP_PROTOCOL_H

#include <cstddef>

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred { class DynamicBuffer; }
namespace brickred::protocol { class HttpMessage; }
namespace brickred::protocol { class HttpRequest; }
namespace brickred::protocol { class HttpResponse; }

namespace brickred::protocol {

class HttpProtocol {
public:
    struct Status {
        enum type {
            READING_START_LINE = 0,
            READING_HEADER,
            READING_BODY,
            FINISHED,
            PENDING_ERROR,
            MAX
        };
    };

    struct RetCode {
        enum type {
            ERROR = -1,
            WAITING_MORE_DATA = 0,
            MESSAGE_READY = 1
        };
    };

    using OutputCallback = Function<void (const char *, size_t)>;

    HttpProtocol();
    ~HttpProtocol();
    void reset();

    Status::type getStatus() const;

    void setOutputCallback(const OutputCallback &output_cb);

    RetCode::type recvMessage(DynamicBuffer *buffer);
    bool retrieveRequest(HttpRequest *request);
    bool retrieveResponse(HttpResponse *response);
    void sendMessage(const HttpMessage &message);
    static void writeMessage(const HttpMessage &message,
                             DynamicBuffer *buffer);

private:
    BRICKRED_NONCOPYABLE(HttpProtocol)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred::protocol

#endif
