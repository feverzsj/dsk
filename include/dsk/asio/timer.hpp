#pragma once

#include <dsk/expected.hpp>
#include <dsk/hosted_async_op.hpp>
#include <dsk/asio/config.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <chrono>


namespace dsk{


template<
    class Clock,
    class WaitTraits = asio::wait_traits<Clock>
>
class basic_waitable_timer :
    public asio::basic_waitable_timer<Clock, WaitTraits, async_op_io_ctx_executor>
{
    using base = asio::basic_waitable_timer<Clock, WaitTraits, async_op_io_ctx_executor>;

public:
    using time_point = typename Clock::time_point;
    using duration   = typename Clock::duration;

    using base::base;

    basic_waitable_timer()
        : base(DSK_DEFAULT_IO_CONTEXT)
    {}

    explicit basic_waitable_timer(time_point const& t)
        : base(DSK_DEFAULT_IO_CONTEXT, t)
    {}

    explicit basic_waitable_timer(duration const& d)
        : base(DSK_DEFAULT_IO_CONTEXT, d)
    {}

    static auto now() noexcept
    {
        return Clock::now();
    }

    auto wait()
    {
        return base::async_wait();
    }

    auto wait_until(time_point const& t)
    {
        base::expires_at(t);
        return wait();
    }

    auto wait_for(duration const& d)
    {
        base::expires_after(d);
        return wait();
    }
};


using steady_timer          = basic_waitable_timer<std::chrono::steady_clock>;
using system_timer          = basic_waitable_timer<std::chrono::system_clock>;
using high_resolution_timer = basic_waitable_timer<std::chrono::high_resolution_clock>;



template<class Timer = steady_timer, class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
auto wait(auto const& td, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return make_hosted_async_op<Timer, Alloc>
    (
        [](auto& timer){ return timer.async_wait(use_async_op); },
        ioc, td
    );
}

template<class Timer = steady_timer, class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
auto wait_until(typename Timer::time_point const& t, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return wait<Timer, Alloc>(t, ioc);
}

template<class Timer = steady_timer, class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
auto wait_for(typename Timer::duration const& d, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return wait<Timer, Alloc>(d, ioc);
}


template<take_cat_e Cat, class Timer, class Op>
struct [[nodiscard]] timed_async_op
{
    DSK_NO_UNIQUE_ADDR Op                     _op;
                       Timer                  _timer;
                       errc                   _err{};
                       atomic<bool>           _done{false};
                       continuation           _cont;
                       std::stop_source       _opSs{std::nostopstate};
                       optional_stop_callback _scb; // must be last defined

    using is_async_op = void;

    timed_async_op(auto&& op, auto& ioc, auto const& td)
        : _op(DSK_FORWARD(op)), _timer(ioc, td)
    {}

    bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        if(set_canceled_if_stop_requested(_err, ctx))
        {
            return false;
        }

        _cont = DSK_FORWARD(cont);

        _opSs = std::stop_source();

        if(stop_possible(ctx))
        {
            _scb.emplace(get_stop_token(ctx), [this]()
            {
                _timer.cancel();
                _opSs.request_stop();
            });
        }

        auto ssCtx = make_async_ctx(ctx, std::ref(_opSs));

        _timer.async_wait([this, ssCtx](auto&& ec) mutable
        {
            if(! ec)
            {
                _err = errc::timeout; // must be assigned before signal _done.
            }

            if(! _done.exchange(true, memory_order_acq_rel)) // first done
            {
                if(! ec)
                {
                    _opSs.request_stop();
                }
            }
            else // last done
            {
                _err = errc();
                resume(mut_move(_cont), ssCtx, _timer.get_executor());
            }
        });

        manual_initiate(_op, ssCtx, [this/*, ssCtx*/]() mutable
        {
            if(! _done.exchange(true, memory_order_acq_rel)) // first done
            {
                _timer.cancel();
            }
            else // last done
            {
                //resume(mut_move(_cont), ssCtx);
                mut_move(_cont)();
            }
        });

        return true;
    }

    constexpr bool is_failed() const noexcept
    {
        return has_err(_err) || dsk::is_failed(_op);
    }

    constexpr decltype(auto) take_result()
    {
        using op_result_t = decltype(take<Cat>(_op));

        if constexpr(std::is_constructible_v<op_result_t, unexpect_t, decltype(_err)>)
        {
            if(has_err(_err)) return op_result_t(unexpect, _err);
            else              return take<Cat>(_op);
        }
        else if constexpr(std::is_convertible_v<decltype(_err), op_result_t>)
        {
            if(has_err(_err)) return op_result_t(_err);
            else              return take<Cat>(_op);
        }
        else
        {
            if(has_err(_err)) return expected<op_result_t, errc>(unexpect, _err);
            else              return expected<op_result_t, errc>(take<Cat>(_op));
        }
    }
};

template<class Timer = steady_timer, take_cat_e Cat = result_cat>
auto wait(auto const& td, _async_op_ auto&& op, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return timed_async_op<Cat, Timer, DSK_DECAY_T(op)>(DSK_FORWARD(op), ioc, td);
}

template<class Timer = steady_timer, take_cat_e Cat = result_cat>
auto wait_until(typename Timer::time_point const& t, _async_op_ auto&& op, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return wait<Timer, Cat>(t, DSK_FORWARD(op), ioc);
}

template<class Timer = steady_timer, take_cat_e Cat = result_cat>
auto wait_for(typename Timer::duration const& d, _async_op_ auto&& op, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
{
    return wait<Timer, Cat>(d, DSK_FORWARD(op), ioc);
}


} // namespace dsk
