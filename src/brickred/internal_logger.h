#ifndef BRICKRED_INTERNAL_LOGGER_H
#define BRICKRED_INTERNAL_LOGGER_H

#include <cstdarg>

#include <brickred/class_util.h>

namespace brickred {

class InternalLogger {
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
    BRICKRED_PRECREATED_SINGLETON(InternalLogger)

    LogFunc log_func_;
};

} // namespace brickred

#define BRICKRED_INTERNAL_LOG_DEBUG(_format, ...) \
    brickred::InternalLogger::getInstance()->log(\
        brickred::InternalLogger::LogLevel::DEBUG, _format, ##__VA_ARGS__)
#define BRICKRED_INTERNAL_LOG_WARNING(_format, ...) \
    brickred::InternalLogger::getInstance()->log(\
        brickred::InternalLogger::LogLevel::WARNING, _format, ##__VA_ARGS__)
#define BRICKRED_INTERNAL_LOG_ERROR(_format, ...) \
    brickred::InternalLogger::getInstance()->log(\
        brickred::InternalLogger::LogLevel::ERROR, _format, ##__VA_ARGS__)

#endif
