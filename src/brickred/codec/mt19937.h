#ifndef BRICKRED_CODEC_MT19937_H
#define BRICKRED_CODEC_MT19937_H

#include <cstddef>
#include <cstdint>

#include <brickred/class_util.h>
#include <brickred/unique_ptr.h>

namespace brickred::codec {

class Mt19937 {
public:
    Mt19937();
    Mt19937(uint32_t s);
    Mt19937(uint32_t key[], size_t key_size);
    virtual ~Mt19937();

    void seed(uint32_t s);
    void seed(uint32_t key[], size_t key_size);

    // uniformly distributed on range [0, 0xffffffff]
    uint32_t nextInt();
    // uniformly distributed on range [0, max)
    uint32_t nextInt(uint32_t max);
    // uniformly distributed on range [min, max]
    uint32_t nextInt(uint32_t min, uint32_t max);
    // uniformly distributed on range [0, 1)
    double nextDouble();

private:
    BRICKRED_NONCOPYABLE(Mt19937)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred::codec

#endif
