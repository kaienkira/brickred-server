#include <brickred/internal_logger.h>

#include <cstdio>

namespace brickred {

BRICKRED_PRECREATED_SINGLETON_IMPL(InternalLogger)

static void defaultLogFunc(int level, const char *format, va_list args) {
    if (level < (int)InternalLogger::LogLevel::MIN ||
        level >= (int)InternalLogger::LogLevel::MAX) {
        return;
    }

    static const char *log_level_string[] = {
        "DEBUG",
        "WARNING",
        "ERROR",
    };

    ::fprintf(stderr, "[%s] ", log_level_string[level]);
    ::vfprintf(stderr, format, args);
    ::fprintf(stderr, "\n");
}

InternalLogger::InternalLogger() : log_func_(defaultLogFunc)
{
}

InternalLogger::~InternalLogger()
{
}

void InternalLogger::setLogFunc(LogFunc log_func)
{
    log_func_ = log_func;
}

void InternalLogger::log(int level, const char *format, ...)
{
    if (nullptr == log_func_) {
        return;
    }

    va_list args;
    va_start(args, format);
    log_func_(level, format, args);
    va_end(args);
}

} // namespace brickred
