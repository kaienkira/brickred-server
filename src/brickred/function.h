#ifndef BRICKRED_FUNCTION_H
#define BRICKRED_FUNCTION_H

#include <brickred/class_util.h>

namespace brickred {

template <typename FunctionSignature>
class Function;

class NullFunction {};

template <typename R, typename... P>
class Function<R (P...)> final {
public:
    Function() :
        func_(nullptr), obj_(nullptr) {}
    Function(NullFunction) :
        func_(nullptr), obj_(nullptr) {}
    Function(const Function &rhs) :
        func_(rhs.func_), obj_(rhs.obj_) {}

    Function &operator=(NullFunction)
        { func_ = 0; obj_ = 0; return *this; }
    Function &operator=(const Function &rhs)
        { func_ = rhs.func_; obj_ = rhs.obj_; return *this; }

    inline R operator()(P... p) const
    {
        return (*func_)(obj_, p...);
    }

    explicit operator bool() const
    {
        return func_ != 0;
    }

private:
    using FunctionType = R (*)(const void *, P...);

    Function(FunctionType func, const void *obj) :
        func_(func), obj_(obj) {}

    template <typename FR, typename... FP>
    friend class FreeFunctionFactory;
    template <typename FR, class FT, typename... FP>
    friend class MemberFunctionFactory;
    template <typename FR, class FT, typename... FP>
    friend class ConstMemberFunctionFactory;

    FunctionType func_;
    const void *obj_;
};

template <typename R, typename... P>
void operator==(const Function<R (P...)> &, const Function<R (P...)> &) = delete;
template <typename R, typename... P>
void operator!=(const Function<R (P...)> &, const Function<R (P...)> &) = delete;

template <typename R, typename... P>
class FreeFunctionFactory final {
private:
    template <R (*func)(P...)>
    static R wrapper(const void *, P... p)
    {
        return (*func)(p...);
    }

public:
    template <R (*func)(P...)>
    inline static Function<R (P...)> bind()
    {
        return Function<R (P...)>(&FreeFunctionFactory::template wrapper<func>, 0);
    }
};

template <typename R, typename... P>
inline FreeFunctionFactory<R, P...> _getFunctionFactory(R (*)(P...))
{
    return FreeFunctionFactory<R, P...>();
}

template <typename R, class T, typename... P>
class MemberFunctionFactory final {
private:
    template <R (T::*func)(P...)>
    static R wrapper(const void *o, P... p)
    {
        T *obj = const_cast<T *>(static_cast<const T *>(o));
        return (obj->*func)(p...);
    }

public:
    template <R (T::*func)(P...)>
    inline static Function<R (P...)> bind(T *o)
    {
        return Function<R (P...)>(&MemberFunctionFactory::template wrapper<func>,
                                  static_cast<const void *>(o));
    }
};

template <typename R, class T, typename... P>
inline MemberFunctionFactory<R, T, P...> _getFunctionFactory(R (T::*)(P...))
{
    return MemberFunctionFactory<R, T, P...>();
}

template <typename R, class T, typename... P>
class ConstMemberFunctionFactory final {
private:
    template <R (T::*func)(P...) const>
    static R wrapper(const void *o, P... p)
    {
        const T *obj = static_cast<const T *>(o);
        return (obj->*func)(p...);
    }

public:
    template <R (T::*func)(P...) const>
    inline static Function<R (P...)> bind(const T *o)
    {
        return Function<R (P...)>(&ConstMemberFunctionFactory::template wrapper<func>,
                                  static_cast<const void *>(o));
    }
};

template <typename R, class T, typename... P>
inline ConstMemberFunctionFactory<R, T, P...> _getFunctionFactory(R (T::*)(P...) const)
{
    return ConstMemberFunctionFactory<R, T, P...>();
}

#define BRICKRED_BIND_FREE_FUNC(_free_func_ptr) \
    brickred::_getFunctionFactory(_free_func_ptr).bind<_free_func_ptr>()

#define BRICKRED_BIND_MEM_FUNC(_mem_func_ptr, _instance_ptr) \
    brickred::_getFunctionFactory(_mem_func_ptr).bind<_mem_func_ptr>(_instance_ptr)

#define BRICKRED_BIND_TEMPLATE_MEM_FUNC(_mem_func_ptr, _instance_ptr) \
    brickred::_getFunctionFactory(_mem_func_ptr).template bind<_mem_func_ptr>(_instance_ptr)

} // namespace brickred

#endif
