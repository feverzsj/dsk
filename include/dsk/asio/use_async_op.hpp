#pragma once

#include <dsk/expected.hpp>
#include <dsk/async_op.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/asio/config.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <memory>


namespace dsk{


struct use_async_op_t
{
    template<class Ex>
    struct executor_with_default : Ex
    {
        using default_completion_token_type = use_async_op_t;

        using Ex::Ex;

        executor_with_default(Ex const& ex)
            : Ex(ex)
        {}
    };

    template<class T>
    using as_default_on_t = typename T::template rebind_executor<
                                executor_with_default<typename T::executor_type>>::other;
    
    template<class T>
    using as_default_with_io_ctx_executor_t = typename T::template rebind_executor<
                                                 executor_with_default<asio::io_context::executor_type>>::other;

    static auto as_default_on(auto&& t)
    {
        return as_default_on_t<DSK_NO_CVREF_T(t)>(DSK_FORWARD(t));
    }
};

inline constexpr use_async_op_t use_async_op;


using async_op_any_io_executor = use_async_op_t::executor_with_default<asio::any_io_executor>;
using async_op_any_io_strand   = use_async_op_t::executor_with_default<asio::strand<asio::any_io_executor>>;
using async_op_io_ctx_executor = use_async_op_t::executor_with_default<asio::io_context::executor_type>;
using async_op_io_ctx_strand   = use_async_op_t::executor_with_default<asio::strand<asio::io_context::executor_type>>;


template<class T>
using use_async_op_as_default_on_t = typename use_async_op_t::template as_default_on_t<T>;

template<class T>
using use_async_op_as_default_with_io_ctx_executor_t = typename use_async_op_t::template as_default_with_io_ctx_executor_t<T>;


// force the result type to be expected<T>
template<class T>
struct use_async_op_for_t{};

template<class T>
inline constexpr use_async_op_for_t<T> use_async_op_for;


template<class T>
concept _asio_executor_ = asio::execution::is_executor_v<std::remove_cvref_t<T>>;

template<class T>
concept _asio_execution_context_ = _derived_from_<T, asio::execution_context>;


constexpr void resume(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr, _asio_execution_context_ auto& exc)
{
    resume(DSK_FORWARD(cont), DSK_FORWARD(sr), static_cast<void*>(std::addressof(exc)));
}

constexpr void resume(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr, _asio_executor_ auto&& ex)
{
    resume(DSK_FORWARD(cont), DSK_FORWARD(sr), asio::query(ex, asio::execution::context));
}


template<_expected_ R, class Init, class... Args>
struct [[nodiscard]] async_op_for_use_async_op
{
    DSK_NO_UNIQUE_ADDR Init           _init;
    DSK_NO_UNIQUE_ADDR tuple<Args...> _args;
    std::optional<R>                  _r;
    continuation                      _cont;
    optional_stop_callback            _scb; // must be last one defined

    explicit async_op_for_use_async_op(auto&& init, auto&&... args)
        : _init(DSK_FORWARD(init)), _args(DSK_FORWARD(args)...)
    {}

    using is_async_op = void;

    bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        DSK_ASSERT(! _r);

        if(stop_requested(ctx))
        {
            _r.emplace(unexpect, errc::canceled);
            return false;
        }

        _cont = DSK_FORWARD(cont);

        apply_elms(_args, [&](auto&... args)
        {
            auto onComp = [this, ctx](auto&& u, auto&&... vs) mutable
            {
                if constexpr(_err_code_or_enum_<decltype(u)>)
                {
                    if(! has_err(u)) _r.emplace(expect, DSK_FORWARD(vs)...);
                    else             _r.emplace(unexpect, DSK_FORWARD(u));
                }
                else
                {
                    _r.emplace(expect, DSK_FORWARD(u), DSK_FORWARD(vs)...);
                }

                resume(mut_move(_cont), ctx, asio::get_associated_executor(_init));
            };

            if(stop_possible(ctx))
            {
                struct data_t
                {
                    atomic<bool> inited{false};
                    asio::cancellation_signal cancelSig;
                };

                auto sd = std::make_shared<data_t>();

                _scb.emplace(get_stop_token(ctx), [sd]()
                {
                    if(sd->inited.load(std::memory_order_acquire))
                    {
                        sd->cancelSig.emit(asio::cancellation_type::all);
                    }
                });

                mut_move(_init)(
                    asio::bind_cancellation_slot(sd->cancelSig.slot(), mut_move(onComp)),
                    std::move(args)...);

                sd->inited.store(true, std::memory_order_release);
            }
            else
            {
                mut_move(_init)(mut_move(onComp), std::move(args)...);
            }
        });

        return true;
    }

    bool is_failed() const noexcept
    {
        DSK_ASSERT(_r);
        return has_err(*_r);
    }

    auto take_result() noexcept
    {
        DSK_ASSERT(_r);
        return *mut_move(_r);
    }
};


} // namespace dsk


namespace boost::asio{


template<class R, dsk::_err_code_or_enum_ E, class... Vs>
class async_result<dsk::use_async_op_t, R(E, Vs...)>
{
public:
    static auto initiate(auto&& init, dsk::use_async_op_t, auto&&... args)
    {
        using r_type = dsk::expected<
                        DSK_CONDITIONAL_T(sizeof...(Vs) == 0, void,
                        DSK_CONDITIONAL_T(sizeof...(Vs) == 1, Vs...,
                                                              dsk::tuple<Vs...>))>;

        return dsk::make_templated<dsk::async_op_for_use_async_op, r_type>(DSK_FORWARD(init), DSK_FORWARD(args)...);
    }
};

template<class T, class R, dsk::_err_code_or_enum_ E, class... Vs>
class async_result<dsk::use_async_op_for_t<T>, R(E, Vs...)>
{
public:
    static auto initiate(auto&& init, dsk::use_async_op_for_t<T>, auto&&... args)
    {
        using r_type = dsk::expected<T>;

        return dsk::make_templated<dsk::async_op_for_use_async_op, r_type>(DSK_FORWARD(init), DSK_FORWARD(args)...);
    }
};

template<class R, class... Vs>
class async_result<dsk::use_async_op_t, R(Vs...)>
    : public async_result<dsk::use_async_op_t, R(dsk::error_code, Vs...)>
{};

template<class T, class R, class... Vs>
class async_result<dsk::use_async_op_for_t<T>, R(Vs...)>
    : public async_result<dsk::use_async_op_for_t<T>, R(dsk::error_code, Vs...)>
{};


} // namespace boost::asio
