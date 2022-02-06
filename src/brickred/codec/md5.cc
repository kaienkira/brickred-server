#include <brickred/codec/md5.h>

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace brickred::codec {

Md5::Md5()
{
    reset();
}

Md5::~Md5()
{
}

void Md5::reset()
{
    hash_[0] = 0x67452301;
    hash_[1] = 0xefcdab89;
    hash_[2] = 0x98badcfe;
    hash_[3] = 0x10325476;
    message_size_ = 0;
}

static void md5ProcessBlock(uint32_t *hash, const uint8_t *work_block)
{
    #define ROTLEFT(_a, _b) (((_a) << (_b)) | ((_a) >> (32 - (_b))))
    #define F(_x, _y, _z) (((_x) & (_y)) | (~(_x) & (_z)))
    #define G(_x, _y, _z) (((_x) & (_z)) | ((_y) & ~(_z)))
    #define H(_x, _y, _z) ((_x) ^ (_y) ^ (_z))
    #define I(_x, _y, _z) ((_y) ^ ((_x) | ~(_z)))

    #define FF(_a, _b, _c, _d, _m, _s, _t) \
    {                                      \
        _a += F(_b, _c, _d) + _m + _t;     \
        _a = _b + ROTLEFT(_a, _s);         \
    }

    #define GG(_a, _b, _c, _d, _m, _s, _t) \
    {                                      \
        _a += G(_b, _c, _d) + _m + _t;     \
        _a = _b + ROTLEFT(_a, _s);         \
    }

    #define HH(_a, _b, _c, _d, _m, _s, _t) \
    {                                      \
        _a += H(_b, _c, _d) + _m + _t;     \
        _a = _b + ROTLEFT(_a, _s);         \
    }

    #define II(_a, _b, _c, _d, _m, _s, _t) \
    {                                      \
        _a += I(_b, _c, _d) + _m + _t;     \
        _a = _b + ROTLEFT(_a, _s);         \
    }


    uint32_t a = hash[0];
    uint32_t b = hash[1];
    uint32_t c = hash[2];
    uint32_t d = hash[3];
    uint32_t w[64];

    // init arary w
    for (int i = 0; i < 16; ++i) {
        w[i] = (work_block[i * 4 + 3] << 24) |
               (work_block[i * 4 + 2] << 16) |
               (work_block[i * 4 + 1] << 8) |
               (work_block[i * 4]);
    }

    FF(a, b, c, d, w[0],   7, 0xd76aa478);
    FF(d, a, b, c, w[1],  12, 0xe8c7b756);
    FF(c, d, a, b, w[2],  17, 0x242070db);
    FF(b, c, d, a, w[3],  22, 0xc1bdceee);
    FF(a, b, c, d, w[4],   7, 0xf57c0faf);
    FF(d, a, b, c, w[5],  12, 0x4787c62a);
    FF(c, d, a, b, w[6],  17, 0xa8304613);
    FF(b, c, d, a, w[7],  22, 0xfd469501);
    FF(a, b, c, d, w[8],   7, 0x698098d8);
    FF(d, a, b, c, w[9],  12, 0x8b44f7af);
    FF(c, d, a, b, w[10], 17, 0xffff5bb1);
    FF(b, c, d, a, w[11], 22, 0x895cd7be);
    FF(a, b, c, d, w[12],  7, 0x6b901122);
    FF(d, a, b, c, w[13], 12, 0xfd987193);
    FF(c, d, a, b, w[14], 17, 0xa679438e);
    FF(b, c, d, a, w[15], 22, 0x49b40821);

    GG(a, b, c, d, w[1],   5, 0xf61e2562);
    GG(d, a, b, c, w[6],   9, 0xc040b340);
    GG(c, d, a, b, w[11], 14, 0x265e5a51);
    GG(b, c, d, a, w[0],  20, 0xe9b6c7aa);
    GG(a, b, c, d, w[5],   5, 0xd62f105d);
    GG(d, a, b, c, w[10],  9, 0x02441453);
    GG(c, d, a, b, w[15], 14, 0xd8a1e681);
    GG(b, c, d, a, w[4],  20, 0xe7d3fbc8);
    GG(a, b, c, d, w[9],   5, 0x21e1cde6);
    GG(d, a, b, c, w[14],  9, 0xc33707d6);
    GG(c, d, a, b, w[3],  14, 0xf4d50d87);
    GG(b, c, d, a, w[8],  20, 0x455a14ed);
    GG(a, b, c, d, w[13],  5, 0xa9e3e905);
    GG(d, a, b, c, w[2],   9, 0xfcefa3f8);
    GG(c, d, a, b, w[7],  14, 0x676f02d9);
    GG(b, c, d, a, w[12], 20, 0x8d2a4c8a);

    HH(a, b, c, d, w[5],   4, 0xfffa3942);
    HH(d, a, b, c, w[8],  11, 0x8771f681);
    HH(c, d, a, b, w[11], 16, 0x6d9d6122);
    HH(b, c, d, a, w[14], 23, 0xfde5380c);
    HH(a, b, c, d, w[1],   4, 0xa4beea44);
    HH(d, a, b, c, w[4],  11, 0x4bdecfa9);
    HH(c, d, a, b, w[7],  16, 0xf6bb4b60);
    HH(b, c, d, a, w[10], 23, 0xbebfbc70);
    HH(a, b, c, d, w[13],  4, 0x289b7ec6);
    HH(d, a, b, c, w[0],  11, 0xeaa127fa);
    HH(c, d, a, b, w[3],  16, 0xd4ef3085);
    HH(b, c, d, a, w[6],  23, 0x04881d05);
    HH(a, b, c, d, w[9],   4, 0xd9d4d039);
    HH(d, a, b, c, w[12], 11, 0xe6db99e5);
    HH(c, d, a, b, w[15], 16, 0x1fa27cf8);
    HH(b, c, d, a, w[2],  23, 0xc4ac5665);

    II(a, b, c, d, w[0],   6, 0xf4292244);
    II(d, a, b, c, w[7],  10, 0x432aff97);
    II(c, d, a, b, w[14], 15, 0xab9423a7);
    II(b, c, d, a, w[5],  21, 0xfc93a039);
    II(a, b, c, d, w[12],  6, 0x655b59c3);
    II(d, a, b, c, w[3],  10, 0x8f0ccc92);
    II(c, d, a, b, w[10], 15, 0xffeff47d);
    II(b, c, d, a, w[1],  21, 0x85845dd1);
    II(a, b, c, d, w[8],   6, 0x6fa87e4f);
    II(d, a, b, c, w[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, w[6],  15, 0xa3014314);
    II(b, c, d, a, w[13], 21, 0x4e0811a1);
    II(a, b, c, d, w[4],   6, 0xf7537e82);
    II(d, a, b, c, w[11], 10, 0xbd3af235);
    II(c, d, a, b, w[2],  15, 0x2ad7d2bb);
    II(b, c, d, a, w[9],  21, 0xeb86d391);

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;

    #undef II
    #undef HH
    #undef GG
    #undef FF
    #undef I
    #undef H
    #undef G
    #undef F
    #undef ROTLEFT
}

void Md5::update(const char *buffer, size_t size)
{
    size_t wb_size = message_size_ & 63;
    message_size_ += size;

    // fill partial work block until it get full
    if (wb_size > 0) {
        size_t left = 64 - wb_size;
        ::memcpy(work_block_ + wb_size, buffer, std::min(size, left));

        // not full, wait for more data
        if (size < left) {
            return;
        }
        // process work block
        md5ProcessBlock(hash_, work_block_);
        buffer += left;
        size -= left;
    }

    // process left 64-byte block data without copy
    while (size >= 64) {
        md5ProcessBlock(hash_, (uint8_t *)buffer);
        buffer += 64;
        size -= 64;
    }

    // save leftover to work block
    if (size > 0) {
        ::memcpy(work_block_, buffer, size);
    }
}

void Md5::digest(char hash[16])
{
    // pad with a 0x80, then 0x0, then length
    static const uint8_t pad[64] = {0x80};
    uint8_t pad_len[8];
    uint64_t bit_size = message_size_ * 8;
    pad_len[7] = bit_size >> 56;
    pad_len[6] = bit_size >> 48;
    pad_len[5] = bit_size >> 40;
    pad_len[4] = bit_size >> 32;
    pad_len[3] = bit_size >> 24;
    pad_len[2] = bit_size >> 16;
    pad_len[1] = bit_size >> 8;
    pad_len[0] = bit_size;

    size_t wb_size = message_size_ & 63;
    if (wb_size > 55) {
        // no room left to store 8-byte pad length + 1-byte 0x80
        // store pad length at next block
        update((const char *)pad, 64 - wb_size + 56);
        update((const char *)pad_len, 8);
    } else {
        update((const char *)pad, 56 - wb_size);
        update((const char *)pad_len, 8);
    }

    for (int i = 0; i < 4; ++i) {
        hash[i * 4 + 3] = hash_[i] >> 24;
        hash[i * 4 + 2] = hash_[i] >> 16;
        hash[i * 4 + 1] = hash_[i] >> 8;
        hash[i * 4] = hash_[i];
    }
}

std::string Md5::digest()
{
    char hash[16];
    digest(hash);

    char hex_output[64];
    int count = 0;
    for (size_t i = 0; i < sizeof(hash); ++i) {
        count += ::snprintf(hex_output + count,
            sizeof(hex_output) - count, "%02hhx", (unsigned char)hash[i]);
    }
    return std::string(hex_output);
}

std::string md5(const std::string &str)
{
    return md5(str.c_str(), str.size());
}

std::string md5(const char *buffer, size_t size)
{
    Md5 ctx;

    ctx.update(buffer, size);

    return ctx.digest();
}

std::string md5Binary(const std::string &str)
{
    return md5Binary(str.c_str(), str.size());
}

std::string md5Binary(const char *buffer, size_t size)
{
    Md5 ctx;
    char hash[16];

    ctx.update(buffer, size);
    ctx.digest(hash);

    return std::string(hash, sizeof(hash));
}

} // namespace brickred::codec
