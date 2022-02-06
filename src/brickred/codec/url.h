#ifndef BRICKRED_CODEC_URL_H
#define BRICKRED_CODEC_URL_H

#include <cstddef>
#include <string>

namespace brickred::codec {

int urlEncode(const char *in, size_t in_size,
              char *out, size_t out_size);
std::string urlEncode(const std::string &str);
std::string urlEncode(const char *buffer, size_t size);

int urlDecode(const char *in, size_t in_size,
              char *out, size_t out_size);
std::string urlDecode(const std::string &str);
std::string urlDecode(const char *buffer, size_t size);

} // namespace brickred::codec

#endif
