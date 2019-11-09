#include <brickred/random.h>

#include <brickred/timestamp.h>

namespace brickred {

Random::Random()
{
    Timestamp now;
    now.setNow();
    seed(now.getSecond() +
         ((now.getSecond() & 0x1) ? -1 : 1) *
         now.getMilliSecond() * 24 * 60 * 60);
}

Random::Random(uint32_t s) :
    Mt19937(s)
{
}

Random::Random(uint32_t key[], size_t key_length) :
    Mt19937(key, key_length)
{
}

Random::~Random()
{
}

} // namespace brickred
