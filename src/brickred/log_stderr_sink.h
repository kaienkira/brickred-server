#ifndef BRICKRED_LOG_STDERR_SINK_H
#define BRICKRED_LOG_STDERR_SINK_H

#include <cstddef>

#include <brickred/class_util.h>
#include <brickred/log_sink.h>

namespace brickred {

class LogStderrSink final : public LogSink {
public:
    LogStderrSink();
    ~LogStderrSink() override;

    void log(const char *buffer, size_t size) override;

private:
    BRICKRED_NONCOPYABLE(LogStderrSink)
};

} // namespace brickred

#endif
