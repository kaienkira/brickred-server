#ifndef BRICKRED_RANDOM_H
#define BRICKRED_RANDOM_H

#include <brickred/class_util.h>
#include <brickred/codec/mt19937.h>

namespace brickred {

class Random final : public codec::Mt19937 {
public:
    using codec::Mt19937::Mt19937;
    Random();
    ~Random() override;

private:
    BRICKRED_NONCOPYABLE(Random)
};

} // namespace brickred

#endif
