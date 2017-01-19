#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <brickred/function.h>
#include <brickred/io_service.h>

using namespace brickred;

class TimerHandler {
public:
    int timeout_ms_;

    void onTimeout(int64_t timer_id) {
        ::printf("%lu:%d\n", timer_id, timeout_ms_);
    }
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        ::fprintf(stderr, "usage: %s <timer_num>\n", argv[0]);
        return -1;
    }

    srand(time(NULL));

    IOService io_service;
    int timer_num = ::atoi(argv[1]);

    TimerHandler *timer_handlers = new TimerHandler[timer_num];

    for (int i = 0; i < timer_num; ++i) {
        timer_handlers[i].timeout_ms_ = rand() % 1000 * 10;
        io_service.startTimer(timer_handlers[i].timeout_ms_,
            BRICKRED_BIND_MEM_FUNC(&TimerHandler::onTimeout,
                                   &timer_handlers[i]), 1);
    }

    io_service.loop();

    delete[] timer_handlers;

    return 0;
}
