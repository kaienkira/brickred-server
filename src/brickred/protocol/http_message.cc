#include <brickred/protocol/http_message.h>

#include <cstddef>
#include <utility>

#include <brickred/timestamp.h>

namespace brickred::protocol {

static const std::string s_cstr_empty_string;
static const std::string s_cstr_http_1_1("HTTP/1.1");
static const std::string s_cstr_http_1_0("HTTP/1.0");
static const std::string s_cstr_unknown("UNKNOWN");

HttpMessage::HttpMessage() :
    message_type_(MessageType::UNKNOWN),
    version_(Version::UNKNOWN)
{
}

HttpMessage::~HttpMessage()
{
}

void HttpMessage::swap(HttpMessage &other)
{
    std::swap(message_type_, other.message_type_);
    std::swap(version_, other.version_);
    headers_.swap(other.headers_);
    body_.swap(other.body_);
}

const std::string &HttpMessage::getHeader(const std::string &key) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return s_cstr_empty_string;
    } else {
        const std::vector<std::string> &header_list = iter->second;
        if (header_list.empty()) {
            return s_cstr_empty_string;
        } else {
            return header_list[0];
        }
    }
}

const std::vector<std::string> *HttpMessage::getHeaderList(
    const std::string &key) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return nullptr;
    } else {
        return &iter->second;
    }
}

bool HttpMessage::hasHeader(const std::string &key) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return false;
    }

    const std::vector<std::string> &header_list = iter->second;
    if (header_list.empty()) {
        return false;
    } else {
        return true;
    }
}

bool HttpMessage::headerEqual(
    const std::string &key, const std::string &value) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return false;
    }

    const std::vector<std::string> &header_list = iter->second;
    if (header_list.size() != 1) {
        return false;
    }

    if (string_util::caseInsensitiveEqual(header_list[0], value)) {
        return true;
    } else {
        return false;
    }
}

bool HttpMessage::headerListOneEqual(
    const std::string &key, const std::string &value) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return false;
    }

    const std::vector<std::string> &header_list = iter->second;
    for (size_t i = 0; i < header_list.size(); ++i) {
        const std::string &header = header_list[i];
        if (string_util::caseInsensitiveEqual(header, value)) {
            return true;
        }
    }

    return false;
}

bool HttpMessage::headerListOneTokenEqual(
    const std::string &key, const std::string &value) const
{
    HeaderMap::const_iterator iter = headers_.find(key);
    if (iter == headers_.end()) {
        return false;
    }
    const std::vector<std::string> &header_list = iter->second;
    for (size_t i = 0; i < header_list.size(); ++i) {
        const std::string &header = header_list[i];
        std::vector<std::string> tokens;
        string_util::split(header.data(), header.size(), ",", &tokens);
        for (size_t j = 0; j < tokens.size(); ++j) {
            std::string token = string_util::trim(tokens[j]);
            if (string_util::caseInsensitiveEqual(token, value)) {
                return true;
            }
        }
    }

    return false;
}

void HttpMessage::setVersion(Version version)
{
    version_ = version;
}

void HttpMessage::setHeader(const std::string &key, const std::string &value)
{
    std::pair<HeaderMap::iterator, bool> p = headers_.try_emplace(key);
    std::vector<std::string> &header_list = p.first->second;
    std::string v = value;
    header_list.clear();
    header_list.push_back(v);
}

void HttpMessage::addHeader(const std::string &key, const std::string &value)
{
    std::pair<HeaderMap::iterator, bool> p = headers_.try_emplace(key);
    std::vector<std::string> &header_list = p.first->second;
    header_list.push_back(value);
}

void HttpMessage::removeHeader(const std::string &key)
{
    headers_.erase(key);
}

void HttpMessage::setBody(const char *body, size_t size)
{
    body_.assign(body, size);
}

void HttpMessage::setBody(const std::string &body)
{
    body_ = body;
}

HttpMessage::Version HttpMessage::VersionStrToEnum(
    const std::string &version_str)
{
    if (version_str == s_cstr_http_1_1) {
        return Version::HTTP_1_1;
    } else if (version_str == s_cstr_http_1_0) {
        return Version::HTTP_1_0;
    } else {
        return Version::UNKNOWN;
    }
}

const std::string &HttpMessage::VersionEnumToStr(Version version_enum)
{

    if (Version::HTTP_1_1 == version_enum) {
        return s_cstr_http_1_1;
    } else if (Version::HTTP_1_0 == version_enum) {
        return s_cstr_http_1_0;
    } else {
        return s_cstr_unknown;
    }
}

bool HttpMessage::checkHeaderKeyValid(const char *key, size_t size)
{
    if (size == 0) {
        return false;
    }

    for (size_t i = 0; i < size; ++i) {
        unsigned char c = static_cast<unsigned char>(key[i]);
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) {
            continue;
        }
        if (c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
            c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
            c == '^' || c == '_' || c == '`' || c == '|' || c == '~') {
            continue;
        }

        return false;
    }

    return true;
}

bool HttpMessage::checkHeaderKeyValid(const std::string &key)
{
    return checkHeaderKeyValid(key.data(), key.size());
}

bool HttpMessage::checkHeaderValueValid(const char *value, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        // only accept tab in control char, and deny del
        if ((c < 0x20 && c != '\t') || c == 0x7f) {
            return false;
        }
    }
    return true;
}

bool HttpMessage::checkHeaderValueValid(const std::string &value)
{
    return checkHeaderValueValid(value.data(), value.size());
}

} // namespace brickred::protocol
