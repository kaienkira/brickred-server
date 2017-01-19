#include <brickred/signal_queue.h>

#include <brickred/exception.h>
#include <brickred/io_service.h>
#include <brickred/self_pipe.h>

namespace brickred {

class SignalQueue::Impl {
public:
    typedef SignalQueue::RecvSignalCallback RecvSignalCallback;

    Impl(IOService &io_service);
    ~Impl();

    IOService *getIOService() const;
    void setRecvSignalCallback(const RecvSignalCallback &recv_signal_cb);
    void push(int signum);

private:
    void pipeReadCallback(IODevice *io_device);

private:
    IOService *io_service_;
    SelfPipe pipe_;
    RecvSignalCallback recv_signal_cb_;
};

///////////////////////////////////////////////////////////////////////////////
SignalQueue::Impl::Impl(IOService &io_service) :
    io_service_(&io_service)
{
    if (pipe_.open() == false ||
        pipe_.setNonblock() == false ||
        pipe_.setCloseOnExec() == false) {
        throw SystemErrorException(
            "create signal queue failed in self pipe init"
        );
    }

    pipe_.setReadCallback(BRICKRED_BIND_MEM_FUNC(
        &SignalQueue::Impl::pipeReadCallback, this));
    pipe_.attachIOService(*io_service_);
}

SignalQueue::Impl::~Impl()
{
}

IOService *SignalQueue::Impl::getIOService() const
{
    return io_service_;
}

void SignalQueue::Impl::setRecvSignalCallback(
    const RecvSignalCallback &recv_signal_cb)
{
    recv_signal_cb_ = recv_signal_cb;
}

void SignalQueue::Impl::push(int signum)
{
    pipe_.write((const char *)&signum, sizeof(signum));
}

void SignalQueue::Impl::pipeReadCallback(IODevice *io_device)
{
    int signum = 0;

    while (pipe_.read((char *)&signum, sizeof(int)) > 0) {
        if (recv_signal_cb_) {
            recv_signal_cb_(signum);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
SignalQueue::SignalQueue(IOService &io_service) :
    pimpl_(new Impl(io_service))
{
}

SignalQueue::~SignalQueue()
{
}

IOService *SignalQueue::getIOService() const
{
    return pimpl_->getIOService();
}

void SignalQueue::setRecvSignalCallback(
    const RecvSignalCallback &recv_signal_cb)
{
    pimpl_->setRecvSignalCallback(recv_signal_cb);
}

void SignalQueue::push(int signum)
{
    pimpl_->push(signum);
}

} // namespace brickred
