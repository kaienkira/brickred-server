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

Random::~Random()
{
}

} // namespace brickred
