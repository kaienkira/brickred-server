#ifndef BRICKRED_MUTEX_H
#define BRICKRED_MUTEX_H

#include <brickred/class_util.h>
#include <brickred/unique_ptr.h>

namespace brickred {

class Mutex final {
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();
    void *nativeHandle();

private:
    BRICKRED_NONCOPYABLE(Mutex)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

class LockGuard final {
public:
    explicit LockGuard(Mutex &m) : mutex_(m) { mutex_.lock(); }
    ~LockGuard() { mutex_.unlock(); }

private:
    BRICKRED_NONCOPYABLE(LockGuard)

    Mutex &mutex_;
};

} // namespace brickred

#endif
