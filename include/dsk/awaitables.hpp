#pragma once

#include <dsk/config.hpp>
#include <dsk/util/concepts.hpp>
#include <coroutine>


namespace dsk{


template<_ref_ T>
struct [[nodiscard]] resume_by_forward : std::suspend_never
{
    T _t;
    
    explicit resume_by_forward(auto&& t) requires(_similar_ref_to_<decltype(t), T>)
        : _t(DSK_FORWARD(t))
    {}
    
    [[nodiscard]] constexpr T await_resume() noexcept { return static_cast<T>(_t); }
};

template<class T> resume_by_forward(T&&) -> resume_by_forward<T&&>;


template<class T> requires(! _ref_<T>)
struct [[nodiscard]] resume_by_return : std::suspend_never
{
    T _t;
    explicit resume_by_return(auto&& t) : _t(DSK_FORWARD(t)) {}
    [[nodiscard]] constexpr T await_resume() { return mut_move(_t); }
};

template<class T> resume_by_return(T) -> resume_by_return<T>;


template<class F>
struct [[nodiscard]] resume_by_invoke : std::suspend_never
{
    F _f;
    explicit resume_by_invoke(auto&& f) : _f(DSK_FORWARD(f)) {}
    [[nodiscard]] constexpr decltype(auto) await_resume() { return _f(); }
};

template<class F> resume_by_invoke(F) -> resume_by_invoke<F>;


template<class F>
struct [[nodiscard]] suspend_by_invoke : std::suspend_always
{
    F _f;
    
    explicit suspend_by_invoke(auto&& f) : _f(DSK_FORWARD(f)) {}

    decltype(auto) await_suspend(auto h) noexcept(noexcept(_f(h))) { return _f(h); }
    decltype(auto) await_suspend(auto  ) noexcept(noexcept(_f())) requires(requires{ _f(); }) { return _f(); }
};

template<class F> suspend_by_invoke(F) -> suspend_by_invoke<F>;


template<class F>
struct [[nodiscard]] invoke_with_async_ctx
{
    F f;
};

template<class F> invoke_with_async_ctx(F) -> invoke_with_async_ctx<F>;


} // namespace dsk
