#ifndef BRICKRED_SIGNAL_QUEUE_H
#define BRICKRED_SIGNAL_QUEUE_H

#include <brickred/class_util.h>
#include <brickred/function.h>
#include <brickred/unique_ptr.h>

namespace brickred { class IOService; }

namespace brickred {

class SignalQueue final {
public:
    using RecvSignalCallback = Function<void (int)>;

    SignalQueue(IOService &io_service);
    ~SignalQueue();

    IOService *getIOService() const;
    void setRecvSignalCallback(const RecvSignalCallback &recv_signal_cb);
    void push(int signum);

private:
    BRICKRED_NONCOPYABLE(SignalQueue)

    class Impl;
    UniquePtr<Impl> pimpl_;
};

} // namespace brickred

#endif
