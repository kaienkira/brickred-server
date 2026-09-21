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

    using HeaderMap = std::map<std::string, std::vector<std::string>,
                               string_util::CaseInsensitiveLess>;

    HttpMessage();
    virtual ~HttpMessage() = 0;
    void swap(HttpMessage &other);

    MessageType getMessageType() const { return message_type_; }
    Version getVersion() const { return version_; }
    const HeaderMap &getHeaders() const { return headers_; }
    const std::string &getHeader(const std::string &key) const;
    const std::vector<std::string> *getHeaderList(
        const std::string &key) const;
    bool hasHeader(const std::string &key) const;
    bool headerEqual(
        const std::string &key, const std::string &value) const;
    bool headerListOneEqual(
        const std::string &key, const std::string &value) const;
    bool headerListOneTokenEqual(
        const std::string &key, const std::string &value) const;
    const std::string &getBody() const { return body_; }
    const HeaderMap &getTrailers() const { return trailers_; }
    const std::string &getTrailer(const std::string &key) const;
    const std::vector<std::string> *getTrailerList(
        const std::string &key) const;

    void setVersion(Version version);
    void setHeader(const std::string &key, const std::string &value);
    void addHeader(const std::string &key, const std::string &value);
    void removeHeader(const std::string &key);
    void setBody(const char *body, size_t size);
    void setBody(const std::string &body);
    void setTrailer(const std::string &key, const std::string &value);
    void addTrailer(const std::string &key, const std::string &value);
    void removeTrailer(const std::string &key);

    static Version VersionStrToEnum(const std::string &version_str);
    static const std::string &VersionEnumToStr(Version version_enum);
    static bool checkHeaderKeyValid(const char *key, size_t size);
    static bool checkHeaderKeyValid(const std::string &key);
    static bool checkHeaderValueValid(const char *value, size_t size);
    static bool checkHeaderValueValid(const std::string &value);

protected:
    MessageType message_type_;
    Version version_;
    HeaderMap headers_;
    std::string body_;
    HeaderMap trailers_;
};

} // namespace brickred::protocol

#endif
