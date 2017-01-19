#include <brickred/log_core.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <vector>

#include <brickred/log_sink.h>

namespace brickred {

namespace log_core_impl {

class Logger {
public:
    typedef LogCore::LogFormatter LogFormatter;
    typedef std::vector<LogSink *> LogSinkVector;
    typedef std::vector<int> LogLevelVector;
    typedef std::vector<LogFormatter> LogFormatterVector;

    explicit Logger(LogFormatter formatter, int level_filter,
                    int max_log_size);
    ~Logger();

    bool addSink(LogSink *sink, LogFormatter formatter, int level_filter);
    void log(int level, const char *filename, int line,
             const char *function, const char *format, va_list args);
    void plainLog(int level, const char *format, va_list args);
    void setLevelFilter(int level_filter) { level_filter_ = level_filter; }

private:
    LogFormatter formatter_;
    int level_filter_;
    int max_log_size_;
    LogSinkVector sinks_;
    LogFormatterVector sink_formatters_;
    LogLevelVector sink_level_filters_;
};

///////////////////////////////////////////////////////////////////////////////
Logger::Logger(LogFormatter formatter, int level_filter,
               int max_log_size) :
    formatter_(formatter), level_filter_(level_filter),
    max_log_size_(max_log_size)
{
}

Logger::~Logger()
{
    for (size_t i = 0; i < sinks_.size(); ++i) {
        delete sinks_[i];
    }
}

bool Logger::addSink(LogSink *sink, LogFormatter formatter, int level_filter)
{
    sinks_.reserve(sinks_.size() + 1);
    sink_level_filters_.reserve(sink_formatters_.size() + 1);
    sink_level_filters_.reserve(sink_level_filters_.size() + 1);

    sinks_.push_back(sink);
    sink_formatters_.push_back(formatter);
    sink_level_filters_.push_back(level_filter);

    return true;
}

void Logger::log(int level, const char *filename, int line,
                 const char *function, const char *format, va_list args)
{
    if (level < level_filter_) {
        return;
    }

    UniquePtr<char []> buffer(new char[max_log_size_]);
    size_t count = 0;
    bool buffer_ready = false;

    for (size_t i = 0; i < sinks_.size(); ++i) {
        if (level < sink_level_filters_[i]) {
            continue;
        }

        // lazy format
        if (!buffer_ready) {
            LogFormatter formatter = NULL;

            if (sink_formatters_[i] != NULL) {
                formatter = sink_formatters_[i];
            } else if (formatter_ != NULL) {
                formatter = formatter_;
            }

            if (NULL == formatter) {
                count = ::vsnprintf(buffer.get(), max_log_size_,
                                    format, args);
            } else {
                count = formatter(buffer.get(), max_log_size_,
                                  level, filename, line, function,
                                  format, args);
            }
            count = std::min(count, (size_t)max_log_size_);
            buffer_ready = true;
        }

        sinks_[i]->log(buffer.get(), count);
    }
}

void Logger::plainLog(int level, const char *format, va_list args)
{
    if (level < level_filter_) {
        return;
    }

    UniquePtr<char []> buffer(new char[max_log_size_]);
    size_t count = 0;
    bool buffer_ready = false;

    for (size_t i = 0; i < sinks_.size(); ++i) {
        if (level < sink_level_filters_[i]) {
            continue;
        }

        // lazy format
        if (!buffer_ready) {
            count = ::vsnprintf(buffer.get(), max_log_size_, format, args);
            count = std::min(count, (size_t)max_log_size_);
            buffer_ready = true;
        }

        sinks_[i]->log(buffer.get(), count);
    }
}

} using namespace log_core_impl;

///////////////////////////////////////////////////////////////////////////////
class LogCore::Impl {
public:
    typedef LogCore::LogFormatter LogFormatter;
    typedef std::vector<Logger *> LoggerVector;

    Impl();
    ~Impl();

    void setMaxLoggerCount(int count);
    void setMaxLogSize(int size);

    bool registerLogger(int logger_id, LogFormatter formatter,
                        int level_filter);
    void removeLogger(int logger_id);
    bool addSink(int logger_id, LogSink *sink,
                 LogFormatter formatter, int level_filter);

    void log(int logger_id, int level,
             const char *filename, int line, const char *function,
             const char *format, va_list args);
    void plainLog(int logger_id, int level,
                  const char *format, va_list args);

    void setLevelFilter(int logger_id, int level_filter);

private:
    LoggerVector loggers_;
    int max_log_size_;
};

///////////////////////////////////////////////////////////////////////////////
BRICKRED_PRECREATED_SINGLETON_IMPL(LogCore)

LogCore::Impl::Impl() :
    max_log_size_(0)
{
}

LogCore::Impl::~Impl()
{
    for (size_t i = 0; i < loggers_.size(); ++i) {
        delete loggers_[i];
    }
}

void LogCore::Impl::setMaxLoggerCount(int count)
{
    if (count < 0) {
        return;
    }

    if (count < (int)loggers_.size()) {
        for (size_t i = count; i < loggers_.size(); ++i) {
            delete loggers_[i];
        }
    }
    loggers_.resize(count, NULL);
}

void LogCore::Impl::setMaxLogSize(int size)
{
    if (size <= 0) {
        return;
    }

    max_log_size_ = size;
}

bool LogCore::Impl::registerLogger(int logger_id, LogFormatter formatter,
                                   int level_filter)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return false;
    }
    if (loggers_[logger_id] != NULL) {
        return false;
    }

    loggers_[logger_id] = new Logger(formatter, level_filter, max_log_size_);

    return true;
}

void LogCore::Impl::removeLogger(int logger_id)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return;
    }
    if (NULL == loggers_[logger_id]) {
        return;
    }

    delete loggers_[logger_id];
    loggers_[logger_id] = NULL;
}

bool LogCore::Impl::addSink(int logger_id, LogSink *sink,
                            LogFormatter formatter, int level_filter)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return false;
    }
    if (NULL == loggers_[logger_id]) {
        return false;
    }

    return loggers_[logger_id]->addSink(sink, formatter, level_filter);
}

void LogCore::Impl::log(int logger_id, int level, const char *filename,
                        int line, const char *function,
                        const char *format, va_list args)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return;
    }
    if (NULL == loggers_[logger_id]) {
        return;
    }

    loggers_[logger_id]->log(level, filename, line, function, format, args);
}

void LogCore::Impl::plainLog(int logger_id, int level,
                             const char *format, va_list args)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return;
    }
    if (NULL == loggers_[logger_id]) {
        return;
    }

    loggers_[logger_id]->plainLog(level, format, args);
}

void LogCore::Impl::setLevelFilter(int logger_id, int level_filter)
{
    if (logger_id < 0 || logger_id >= (int)loggers_.size()) {
        return;
    }
    if (NULL == loggers_[logger_id]) {
        return;
    }

    loggers_[logger_id]->setLevelFilter(level_filter);
}

///////////////////////////////////////////////////////////////////////////////
LogCore::LogCore() :
    pimpl_(new Impl())
{
    setMaxLoggerCount();
    setMaxLogSize();
}

LogCore::~LogCore()
{
}

void LogCore::setMaxLoggerCount(int count)
{
    pimpl_->setMaxLoggerCount(count);
}

void LogCore::setMaxLogSize(int size)
{
    pimpl_->setMaxLogSize(size);
}

bool LogCore::registerLogger(int logger_id, LogFormatter formatter,
                             int level_filter)
{
    return pimpl_->registerLogger(logger_id, formatter, level_filter);
}

void LogCore::removeLogger(int logger_id)
{
    pimpl_->removeLogger(logger_id);
}

bool LogCore::addSink(int logger_id, LogSink *sink,
                      LogFormatter formatter, int level_filter)
{
    return pimpl_->addSink(logger_id, sink, formatter, level_filter);
}

void LogCore::log(int logger_id, int level,
                  const char *filename, int line, const char *function,
                  const char *format, ...)
{
    va_list args;
    va_start(args, format);
    pimpl_->log(logger_id, level, filename, line, function, format, args);
    va_end(args);
}

void LogCore::log(int logger_id, int level,
                  const char *filename, int line, const char *function,
                  const char *format, va_list args)
{
    pimpl_->log(logger_id, level, filename, line, function, format, args);
}

void LogCore::plainLog(int logger_id, int level,
                       const char *format, ...)
{
    va_list args;
    va_start(args, format);
    pimpl_->plainLog(logger_id, level, format, args);
    va_end(args);
}

void LogCore::plainLog(int logger_id, int level,
                       const char *format, va_list args)
{
    pimpl_->plainLog(logger_id, level, format, args);
}

} // namespace brickred
