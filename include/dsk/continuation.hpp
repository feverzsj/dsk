#pragma once

#include <dsk/config.hpp>
#include <dsk/scheduler.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/variant.hpp>
#include <dsk/util/function.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/allocator.hpp>
#include <coroutine>


namespace dsk{


template<class T> concept _continuation_ = _nullary_callable_<T>;

template<class P> void _coro_handle_impl(std::coroutine_handle<P>&);
template<class T> concept _coro_handle_ = requires(std::remove_cvref_t<T>& t){ dsk::_coro_handle_impl(t); }
                                          && _continuation_<T>; // this is necessary to make _coro_handle_
                                                                // more constrained than _continuation_

class continuation
{
    std::variant<
        null_op_t,
        std::coroutine_handle<>,
        unique_function<void()>
    > _v;

    enum{ none_idx, coro_idx, fn_idx };

    template<_continuation_ T>
    constexpr static size_t idx_of = []()
    {
             if constexpr(    _null_op_<T>) return none_idx;
        else if constexpr(_coro_handle_<T>) return coro_idx;
        else                                return fn_idx;
    }();

public:
    constexpr continuation(std::nullptr_t = nullptr) noexcept
    {}

    constexpr continuation& operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    constexpr continuation(continuation const&) = delete;
    constexpr continuation& operator=(continuation const&) = delete;

    constexpr continuation(continuation&& c)
        : _v(mut_move(c._v))
    {
        c.reset();
    }

    constexpr continuation& operator=(continuation&& c)
    {
        if(this != std::addressof(c))
        {
            _v = mut_move(c._v);
            c.reset();
        }

        return *this;
    }

    constexpr continuation(_continuation_ auto&& c)
        : _v(std::in_place_index<idx_of<decltype(c)>>, DSK_FORWARD(c))
    {}

    constexpr continuation& operator=(_continuation_ auto&& c)
    {
        constexpr auto idx = idx_of<decltype(c)>;

        if(auto* p = std::get_if<idx>(&_v)) *p = DSK_FORWARD(c);
        else                                _v.template emplace<idx>(DSK_FORWARD(c));

        return *this;
    }

    constexpr void reset()
    {
        if(_v.index() != none_idx)
        {
            _v.template emplace<none_idx>();
        }
    }

    constexpr bool valid() const noexcept { return _v.index() != none_idx; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    constexpr bool is_coro_handle() const noexcept { return _v.index() == coro_idx; }
    constexpr auto coro_handle() const noexcept
    {
        if(auto* h = std::get_if<coro_idx>(&_v))
            return *h;
        return std::coroutine_handle<>();
    }

    constexpr void operator()() { resume(); }

    constexpr void resume()
    {
        DSK_ASSERT(valid());

        visit_var(_v, [&](auto& c)
        {
            if constexpr(_coro_handle_<decltype(c)>)
            {
                DSK_ASSERT(c);
            }
            c();
        });
    }

    constexpr auto tail_resume()
    {
        DSK_ASSERT(valid());

        return visit_var(_v, [&](auto& c) -> std::coroutine_handle<>
        {
            if constexpr(_coro_handle_<decltype(c)>)
            {
                DSK_ASSERT(c);
                return c;
            }
            else
            {
                c();
                return std::noop_coroutine();
            }
        });
    }

    constexpr auto operator<=>(std::coroutine_handle<> r) noexcept
    {
        return coro_handle() <=> r;
    }
};


auto tail_resume(_continuation_ auto&& c)
{
    if constexpr(_no_cvref_same_as_<decltype(c), continuation>)
    {
        return c.tail_resume();
    }
    else if constexpr(_coro_handle_<decltype(c)>)
    {
        DSK_ASSERT(c);
        return c;
        //return std::coroutine_handle<>(c ? c : std::noop_coroutine());
    }
    else
    {
        c();
        return std::noop_coroutine();
    }
}


} // namespace dsk
