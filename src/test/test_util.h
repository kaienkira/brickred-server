#ifndef BRICKRED_TEST_TEST_UTIL_H
#define BRICKRED_TEST_TEST_UTIL_H

#include <cstddef>
#include <brickred/class_util.h>
#include <brickred/unique_ptr.h>

namespace test {

class TestTimer {
public:
    TestTimer();
    ~TestTimer();

private:
    BRICKRED_NONCOPYABLE(TestTimer)

    class Impl;
    brickred::UniquePtr<Impl> pimpl_;
};

void hexdump(const char *buffer, size_t size);

} // end of namespace test

#endif
