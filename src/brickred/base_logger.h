#ifndef BRICKRED_BASE_LOGGER_H
#define BRICKRED_BASE_LOGGER_H

#include <cstdarg>

#include <brickred/class_util.h>

namespace brickred {

class BaseLogger {
public:
    struct LogLevel {
        enum type {
            MIN = 0,

            DEBUG = 0,
            WARNING,
            ERROR,

            MAX
        };
    };

    using LogFunc = void (*)(int level, const char *format, va_list args);

    void setLogFunc(LogFunc log_func);
    void log(int level, const char *format, ...);

private:
    BRICKRED_PRECREATED_SINGLETON(BaseLogger)

    LogFunc log_func_;
};

} // namespace brickred

#define BASE_DEBUG(_format, ...) \
    brickred::BaseLogger::getInstance()->log(\
        brickred::BaseLogger::LogLevel::DEBUG, _format, ##__VA_ARGS__)
#define BASE_WARNING(_format, ...) \
    brickred::BaseLogger::getInstance()->log(\
        brickred::BaseLogger::LogLevel::WARNING, _format, ##__VA_ARGS__)
#define BASE_ERROR(_format, ...) \
    brickred::BaseLogger::getInstance()->log(\
        brickred::BaseLogger::LogLevel::ERROR, _format, ##__VA_ARGS__)

#endif
