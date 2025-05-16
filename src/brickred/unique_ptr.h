#ifndef BRICKRED_UNIQUE_PTR_H
#define BRICKRED_UNIQUE_PTR_H

#include <cstddef>

#include <brickred/class_util.h>

namespace brickred {

template <class T>
class UniquePtr final {
public:
    explicit UniquePtr(T *p = nullptr) : px_(p) {}
    ~UniquePtr() { delete px_; }

    T *operator->() const { return px_; }
    T &operator*() const { return *px_; }
    T *get() const { return px_; }

    void swap(UniquePtr &b)
    {
        T *tmp = b.px_;
        b.px_ = px_;
        px_ = tmp;
    }

    void reset(T *p = nullptr)
    {
        UniquePtr<T>(p).swap(*this);
    }

    T *release()
    {
        T *p = px_;
        px_ = nullptr;

        return p;
    }

    explicit operator bool() const
    {
        return px_ != nullptr;
    }

private:
    BRICKRED_NONCOPYABLE(UniquePtr)

    T *px_;
};

template <class T>
class UniquePtr<T[]> final {
public:
    explicit UniquePtr(T *p = nullptr) : px_(p) {}
    ~UniquePtr() { delete[] px_; }

    T &operator[](size_t i) const { return px_[i]; }
    T *get() const { return px_; }

    void swap(UniquePtr &b)
    {
        T *tmp = b.px_;
        b.px_ = px_;
        px_ = tmp;
    }

    void reset(T *p = nullptr)
    {
        UniquePtr<T[]>(p).swap(*this);
    }

    T *release()
    {
        T *p = px_;
        px_ = nullptr;

        return p;
    }

    explicit operator bool() const
    {
        return px_ != nullptr;
    }

private:
    BRICKRED_NONCOPYABLE(UniquePtr)

    T *px_;
};

template <class T>
inline void swap(UniquePtr<T> &a, UniquePtr<T> &b)
{
    a.swap(b);
}

} // namespace brickred

#endif
