#include "test/test_util.h"

#include <time.h>
#include <unistd.h>
#include <sys/times.h>
#include <cstdio>
#include <iostream>
#include <iomanip>

namespace test {

class TestTimer::Impl {
public:
    Impl();
    ~Impl();

private:
    struct timespec start_real_time_;
    struct timespec start_monotonic_time_;
    struct tms start_cpu_time_;
};

///////////////////////////////////////////////////////////////////////////////

TestTimer::TestTimer() :
    pimpl_(new Impl)
{
}

TestTimer::~TestTimer()
{
}

///////////////////////////////////////////////////////////////////////////////

TestTimer::Impl::Impl()
{
    clock_gettime(CLOCK_REALTIME, &start_real_time_);
    clock_gettime(CLOCK_MONOTONIC, &start_monotonic_time_);
    times(&start_cpu_time_);
}

TestTimer::Impl::~Impl()
{
    struct timespec current_real_time;
    struct timespec current_monotonic_time;
    struct tms current_cpu_time;

    clock_gettime(CLOCK_REALTIME, &current_real_time);
    clock_gettime(CLOCK_MONOTONIC, &current_monotonic_time);
    times(&current_cpu_time);

    double clock_tick = sysconf(_SC_CLK_TCK);
    double real_time =
        ((current_real_time.tv_sec -
          start_real_time_.tv_sec) * 1000000000LL +
         (current_real_time.tv_nsec -
          start_real_time_.tv_nsec)
        ) / 1000000000.0;
    double monotonic_time =
        ((current_monotonic_time.tv_sec -
          start_monotonic_time_.tv_sec) * 1000000000LL +
         (current_monotonic_time.tv_nsec -
          start_monotonic_time_.tv_nsec)
        ) / 1000000000.0;
    double user_time =
        ((current_cpu_time.tms_utime + current_cpu_time.tms_cutime) -
         (start_cpu_time_.tms_utime + start_cpu_time_.tms_cutime)
        ) / clock_tick;
    double system_time =
        ((current_cpu_time.tms_stime + current_cpu_time.tms_cstime) -
         (start_cpu_time_.tms_stime + start_cpu_time_.tms_cstime)
        ) / clock_tick;

    std::cout << std::setprecision(6) << std::fixed
              << "real:" << real_time << "s  "
              << "monotonic:" << monotonic_time << "s  "
              << "user:" << user_time << "s  "
              << "system:" << system_time << "s  "
              << std::setprecision(1)
              << "cpu:"
              << (user_time + system_time) / monotonic_time * 100.0 << "%"
              << std::endl;
}

///////////////////////////////////////////////////////////////////////////////

void hexdump(const char *buffer, size_t size)
{
#define LINE_CHAR_COUNT 16
    const char *buffer_end = buffer + size;
    const char *line_start = buffer;

    for (;;) {
        const char *line_end = std::min(line_start + LINE_CHAR_COUNT,
                                        buffer_end);
        if (line_start == line_end) {
            break;
        }

        char output[1024];
        size_t count = 0;
        size_t blank_count = LINE_CHAR_COUNT;

        // hex part
        for (const char *p = line_start; p < line_end; ++p, --blank_count) {
            count += ::snprintf(output + count, sizeof(output), "%02hhx ", *p);
        }
        for (size_t i = 0; i < blank_count; ++i) {
            count += ::snprintf(output + count, sizeof(output), "   ");
        }

        // blank
        count += ::snprintf(output + count, sizeof(output), "    ");

        // acsii part
        for (const char *p = line_start; p < line_end; ++p) {
            count += ::snprintf(output + count, sizeof(output), "%c",
                                ::isprint(*p) ? *p : '.');
        }

        ::printf("%s\n", output);

        line_start = line_end;
    }

#undef LINE_CHAR_COUNT
}

} // end of namespace test
