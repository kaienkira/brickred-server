#ifndef BRICKRED_PROTOCOL_HTTP_MESSAGE_H
#define BRICKRED_PROTOCOL_HTTP_MESSAGE_H

#include <ctime>
#include <cstddef>
#include <map>
#include <string>

#include <brickred/string_util.h>

namespace brickred::protocol {

class HttpMessage {
public:
    enum class Version {
        UNKNOWN  = 0,
        HTTP_1_0,
        HTTP_1_1
    };

    enum class MessageType {
        UNKNOWN = 0,
        REQUEST,
        RESPONSE
    };

    using HeaderMap = std::map<std::string, std::string,
                               string_util::CaseInsensitiveLess>;

    HttpMessage();
    virtual ~HttpMessage() = 0;
    void swap(HttpMessage &other);

    MessageType getMessageType() const { return message_type_; }
    Version getVersion() const { return version_; }
    const HeaderMap &getHeaders() const { return headers_; }
    const std::string &getHeader(const std::string &key) const;
    bool hasHeader(const std::string &key) const;
    bool headerEqual(const std::string &key,
                     const std::string &value) const;
    bool headerContain(const std::string &key,
                       const std::string &value) const;
    const std::string &getBody() const { return body_; }

    void setVersion(Version version);
    void setHeader(const std::string &key, const std::string &value);
    void removeHeader(const std::string &key);
    void setBody(const char *buffer, size_t size);
    void setBody(const std::string &body);

    bool isConnectionKeepAlive() const;
    void setConnectionKeepAlive();
    void setConnectionClose();
    void setDate(time_t now = 0);

    static Version VersionStrToEnum(const std::string &version_str);
    static const std::string &VersionEnumToStr(Version version_enum);

protected:
    MessageType message_type_;
    Version version_;
    HeaderMap headers_;
    std::string body_;
};

} // namespace brickred::protocol

#endif
