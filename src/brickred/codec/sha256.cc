#include <brickred/codec/sha256.h>

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace brickred::codec {

Sha256::Sha256()
{
    reset();
}

Sha256::~Sha256()
{
}

void Sha256::reset()
{
    hash_[0] = 0x6a09e667;
    hash_[1] = 0xbb67ae85;
    hash_[2] = 0x3c6ef372;
    hash_[3] = 0xa54ff53a;
    hash_[4] = 0x510e527f;
    hash_[5] = 0x9b05688c;
    hash_[6] = 0x1f83d9ab;
    hash_[7] = 0x5be0cd19;
    message_size_ = 0;
}

static void sha256ProcessBlock(uint32_t *hash, const uint8_t *work_block)
{
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    #define ROTLEFT(_a, _b) (((_a) << (_b)) | ((_a) >> (32 - (_b))))
    #define ROTRIGHT(_a, _b) (((_a) >> (_b)) | ((_a) << (32 - (_b))))
    #define CH(_x, _y, _z) (((_x) & (_y)) ^ (~(_x) & (_z)))
    #define MAJ(_x, _y, _z) (((_x) & (_y)) ^ ((_x) & (_z)) ^ ((_y) & (_z)))
    #define EP0(_x) (ROTRIGHT(_x, 2) ^ ROTRIGHT(_x, 13) ^ ROTRIGHT(_x, 22))
    #define EP1(_x) (ROTRIGHT(_x, 6) ^ ROTRIGHT(_x, 11) ^ ROTRIGHT(_x, 25))
    #define SIG0(_x) (ROTRIGHT(_x, 7) ^ ROTRIGHT(_x, 18) ^ ((_x) >> 3))
    #define SIG1(_x) (ROTRIGHT(_x, 17) ^ ROTRIGHT(_x, 19) ^ ((_x) >> 10))

    uint32_t a = hash[0];
    uint32_t b = hash[1];
    uint32_t c = hash[2];
    uint32_t d = hash[3];
    uint32_t e = hash[4];
    uint32_t f = hash[5];
    uint32_t g = hash[6];
    uint32_t h = hash[7];
    uint32_t w[64];

    // init arary w
    for (int i = 0; i < 16; ++i) {
        w[i] = (work_block[i * 4] << 24) |
               (work_block[i * 4 + 1] << 16) |
               (work_block[i * 4 + 2] << 8) |
               (work_block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + k[i] + w[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
    hash[5] += f;
    hash[6] += g;
    hash[7] += h;

    #undef SIG0
    #undef SIG1
    #undef EP1
    #undef EP0
    #undef MAJ
    #undef CH
    #undef ROTRIGHT
    #undef ROTLEFT
}

void Sha256::update(const char *buffer, size_t size)
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
        sha256ProcessBlock(hash_, work_block_);
        buffer += left;
        size -= left;
    }

    // process left 64-byte block data without copy
    while (size >= 64) {
        sha256ProcessBlock(hash_, (uint8_t *)buffer);
        buffer += 64;
        size -= 64;
    }

    // save leftover to work block
    if (size > 0) {
        ::memcpy(work_block_, buffer, size);
    }
}

void Sha256::digest(char hash[32])
{
    // pad with a 0x80, then 0x0, then length
    static const uint8_t pad[64] = {0x80};
    uint8_t pad_len[8];
    uint64_t bit_size = message_size_ * 8;
    pad_len[0] = bit_size >> 56;
    pad_len[1] = bit_size >> 48;
    pad_len[2] = bit_size >> 40;
    pad_len[3] = bit_size >> 32;
    pad_len[4] = bit_size >> 24;
    pad_len[5] = bit_size >> 16;
    pad_len[6] = bit_size >> 8;
    pad_len[7] = bit_size;

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

    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = hash_[i] >> 24;
        hash[i * 4 + 1] = hash_[i] >> 16;
        hash[i * 4 + 2] = hash_[i] >> 8;
        hash[i * 4 + 3] = hash_[i];
    }
}

std::string Sha256::digest()
{
    char hash[32];
    digest(hash);

    char hex_output[128];
    int count = 0;
    for (size_t i = 0; i < sizeof(hash); ++i) {
        count += ::snprintf(hex_output + count,
            sizeof(hex_output) - count, "%02hhx", (unsigned char)hash[i]);
    }
    return std::string(hex_output);
}

std::string sha256(const std::string &str)
{
    return sha256(str.c_str(), str.size());
}

std::string sha256(const char *buffer, size_t size)
{
    Sha256 ctx;

    ctx.update(buffer, size);

    return ctx.digest();
}

std::string sha256Binary(const std::string &str)
{
    return sha256Binary(str.c_str(), str.size());
}

std::string sha256Binary(const char *buffer, size_t size)
{
    Sha256 ctx;
    char hash[32];

    ctx.update(buffer, size);
    ctx.digest(hash);

    return std::string(hash, sizeof(hash));
}

} // namespace brickred::codec
