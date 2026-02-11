#include <brickred/thread.h>

#include <pthread.h>
#include <sched.h>

#include <brickred/condition_variable.h>
#include <brickred/exception.h>
#include <brickred/mutex.h>

namespace brickred {

class Thread::Impl {
public:
    Impl();
    ~Impl();

    void start(const ThreadFunc &thread_func);
    bool joinable();
    void join();
    void detach();

public:
    static void *threadProxy(void *arg);

private:
    pthread_t thread_handle_;
    Mutex data_mutex_;
    ConditionVariable join_cond_;
    bool started_;
    bool join_started_;
    bool joined_;
};

///////////////////////////////////////////////////////////////////////////////
Thread::Impl::Impl() :
    thread_handle_(0),
    started_(false), join_started_(false), joined_(false)
{
}

Thread::Impl::~Impl()
{
    if (joinable()) {
        detach();
    }
}

void Thread::Impl::start(const ThreadFunc &thread_func)
{
    LockGuard lock(data_mutex_);

    if (started_) {
        throw SystemErrorException("thread has already started");
    }

    ThreadFunc *thread_func_copy = new ThreadFunc(thread_func);
    if (::pthread_create(&thread_handle_, nullptr, &threadProxy, thread_func_copy) != 0) {
        delete thread_func_copy;
        throw SystemErrorException("create thread failed in pthread_create");
    }

    started_ = true;
}

bool Thread::Impl::joinable()
{
    LockGuard lock(data_mutex_);
    return started_ && !joined_;
}

void Thread::Impl::join()
{
    bool do_join = false;

    {
        LockGuard lock(data_mutex_);
        if (started_ == false) {
            throw SystemErrorException("thread was never started");
        }
        if (::pthread_equal(thread_handle_, pthread_self())) {
            throw SystemErrorException("thread try to join itself");
        }

        do_join = !join_started_;
        if (do_join) {
            join_started_ = true;
        } else {
            while (joined_ == false) {
                join_cond_.wait(data_mutex_);
            }
        }
    }

    if (do_join) {
        ::pthread_join(thread_handle_, nullptr);
        LockGuard lock(data_mutex_);
        joined_ = true;
        join_cond_.notifyAll();
    }
}

void Thread::Impl::detach()
{
    LockGuard lock(data_mutex_);
    if (!join_started_) {
        ::pthread_detach(thread_handle_);
        join_started_ = true;
        joined_ = true;
    }
}

void *Thread::Impl::threadProxy(void *arg)
{
    UniquePtr<ThreadFunc> thread_func(static_cast<ThreadFunc *>(arg));
    (*thread_func)();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
Thread::Thread() :
    pimpl_(new Impl())
{
}

Thread::~Thread()
{
}

void Thread::start(const ThreadFunc &thread_func)
{
    pimpl_->start(thread_func);
}

bool Thread::joinable()
{
    return pimpl_->joinable();
}

void Thread::join()
{
    pimpl_->join();
}

void Thread::detach()
{
    pimpl_->detach();
}

///////////////////////////////////////////////////////////////////////////////
namespace this_thread {

void sleepFor(int ms)
{
    Mutex m;
    ConditionVariable cond;

    LockGuard lock(m);
    cond.waitFor(m, ms);
}

void yield()
{
    ::sched_yield();
}

} // namespace this_thread

} // namespace brickred
