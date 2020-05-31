#ifndef BRICKRED_RANDOM_H
#define BRICKRED_RANDOM_H

#include <cstddef>
#include <cstdint>

#include <brickred/class_util.h>
#include <brickred/codec/mt19937.h>

namespace brickred {

class Random : public codec::Mt19937 {
public:
    Random();
    Random(uint32_t s);
    Random(uint32_t key[], size_t key_length);
    ~Random() override;

private:
    BRICKRED_NONCOPYABLE(Random)
};

} // namespace brickred

#endif
