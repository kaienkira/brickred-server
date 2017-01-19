#ifndef BRICKRED_LOG_CORE_H
#define BRICKRED_LOG_CORE_H

#include <cstddef>
#include <cstdarg>

#include <brickred/class_util.h>
#include <brickred/unique_ptr.h>

namespace brickred {

class LogSink;

class LogCore {
public:
    typedef size_t (*LogFormatter)(
        char *buffer, size_t buffer_size, int level,
        const char *filename, int line, const char *function,
        const char *format, va_list args
    );

    // call by LogCore constructor, default max logger count is 1
    // if you need more loggers,
    // you should call this function before call registerLogger()
    void setMaxLoggerCount(int count = 1);

    // call by LogCore constructor, default log buffer size is 4096,
    // that means if one log is large than 4096 bytes, it will be truncated
    // you should call this function before call registerLogger()
    void setMaxLogSize(int size = 4096);

    // logger_id must be in [0, max_logger_count)
    bool registerLogger(int logger_id, LogFormatter formatter = NULL,
                        int level_filter = -1);
    void removeLogger(int logger_id);
    bool addSink(int logger_id, LogSink *sink,
                 LogFormatter formatter = NULL, int level_filter = -1);

    // log with formatter
    void log(int logger_id, int level,
             const char *filename, int line, const char *function,
             const char *format, ...);
    void log(int logger_id, int level,
             const char *filename, int line, const char *function,
             const char *format, va_list args);
    // log without formatter
    void plainLog(int logger_id, int level,
                  const char *format, ...);
    void plainLog(int logger_id, int level,
                  const char *format, va_list args);

    // change logger level filter
    void setLevelFilter(int logger_id, int level_filter);

private:
    BRICKRED_PRECREATED_SINGLETON(LogCore)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred

#endif
