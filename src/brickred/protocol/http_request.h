#ifndef BRICKRED_PROTOCOL_HTTP_REQUEST_H
#define BRICKRED_PROTOCOL_HTTP_REQUEST_H

#include <string>

#include <brickred/protocol/http_message.h>

namespace brickred::protocol {

class HttpRequest : public HttpMessage {
public:
    struct Method {
        enum type {
            UNKNOWN = 0,
            GET,
            POST
        };
    };

    HttpRequest();
    ~HttpRequest() override;
    void swap(HttpRequest &other);

    Method::type getMethod() const { return method_; }
    const std::string &getRequestUri() const { return request_uri_; }

    void setMethod(Method::type method);
    void setRequestUri(const std::string &request_uri);

    static Method::type MethodStrToEnum(const std::string &method_str);
    static const std::string &MethodEnumToStr(Method::type method_enum);

private:
    Method::type method_;
    std::string request_uri_;
};

} // namespace brickred::protocol

#endif
