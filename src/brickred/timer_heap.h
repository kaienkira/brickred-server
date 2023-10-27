#ifndef BRICKRED_TIMER_HEAP_H
#define BRICKRED_TIMER_HEAP_H

#include <cstdint>

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred { class Timestamp; }

namespace brickred {

class TimerHeap {
public:
    using TimerId = int64_t;
    using TimerCallback = Function<void (TimerId)>;

    TimerHeap();
    ~TimerHeap();

    int64_t getNextTimeoutMillisecond(const Timestamp &now) const;
    TimerId addTimer(const Timestamp &now, int64_t timeout_ms,
                     const TimerCallback &timer_cb,
                     int call_times = -1);
    void removeTimer(TimerId timer_id);
    void checkTimeout(const Timestamp &now);

private:
    BRICKRED_NONCOPYABLE(TimerHeap)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

}  // namespace brickred

#endif
