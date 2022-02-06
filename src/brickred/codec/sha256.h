#ifndef BRICKRED_CODEC_SHA256_H
#define BRICKRED_CODEC_SHA256_H

#include <cstddef>
#include <cstdint>
#include <string>

#include <brickred/class_util.h>

namespace brickred::codec {

class Sha256 {
public:
    Sha256();
    ~Sha256();
    void reset();

    void update(const char *buffer, size_t size);
    void digest(char hash[32]);
    std::string digest();

private:
    BRICKRED_NONCOPYABLE(Sha256)

    uint32_t hash_[8];
    uint8_t work_block_[64];
    uint64_t message_size_;
};

std::string sha256(const std::string &str);
std::string sha256(const char *buffer, size_t size);
std::string sha256Binary(const std::string &str);
std::string sha256Binary(const char *buffer, size_t size);

} // namespace brickred::codec

#endif
