#ifndef BRICKRED_LOG_FILE_SINK_H
#define BRICKRED_LOG_FILE_SINK_H

#include <cstddef>
#include <string>

#include <brickred/class_util.h>
#include <brickred/log_sink.h>
#include <brickred/unique_ptr.h>

namespace brickred {

class LogFileSink : public LogSink {
public:
    LogFileSink(const std::string &file_path);
    virtual ~LogFileSink();

    virtual void log(const char *buffer, size_t size);
    bool openFile();

private:
    BRICKRED_NONCOPYABLE(LogFileSink)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred

#endif
