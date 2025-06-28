#pragma once

#include <dsk/resumer.hpp>
#include <dsk/inline_scheduler.hpp>
#include <dsk/stop_obj.hpp>
#include <dsk/expected.hpp>
#include <dsk/optional.hpp>
#include <dsk/continuation.hpp>
#include <coroutine>


namespace dsk{


template<class T>
concept _awaiter_ = requires(T t, std::coroutine_handle<> h)
{
    {t.await_ready()} -> std::same_as<bool>;
    t.await_suspend(h);
    t.await_resume();
};


// async context should be lightweight copyable object. Copies of same context should represent same entities.
template<class T> concept _async_ctx_ = requires{ typename std::remove_cvref_t<T>::is_async_ctx; };
// async op should be movable object.
template<class T> concept _async_op_ = requires{ typename std::remove_cvref_t<T>::is_async_op; };


struct null_async_ctx{ using is_async_ctx = void; };
struct null_async_op { using is_async_op  = void; };

struct dummy_async_ctx
{
    using is_async_ctx = void;

    auto get_stop_source() { return std::stop_source(); }
    auto get_resumer() { return inline_resumer(); }
    void add_cleanup(_async_op_ auto&&) {}
};


constexpr bool stop_possible(_async_ctx_ auto&& ctx) noexcept
{
    if constexpr(requires{ ctx.get_stop_source(); }) return ctx.get_stop_source().stop_possible();
    else                                             return false;
}

constexpr bool stop_requested(_async_ctx_ auto&& ctx) noexcept
{
    if constexpr(requires{ ctx.get_stop_source(); }) return ctx.get_stop_source().stop_requested();
    else                                             return false;
}

constexpr auto get_stop_token(_async_ctx_ auto&& ctx) noexcept
{
    if constexpr(requires{ ctx.get_stop_source(); }) return ctx.get_stop_source().get_token();
    else                                             return std::stop_token();
}

constexpr decltype(auto) get_stop_source(_async_ctx_ auto&& ctx) noexcept
{
    if constexpr(requires{ ctx.get_stop_source(); }) return ctx.get_stop_source();
    else                                             return std::stop_source(std::nostopstate);
}


constexpr decltype(auto) get_resumer(_async_ctx_ auto&& ctx) noexcept
{
    if constexpr(requires{ ctx.get_resumer(); }) return ctx.get_resumer();
    else                                         return inline_resumer();
}

constexpr void resume(_continuation_ auto&& cont, _async_ctx_ auto&& ctx, auto&&... args) noexcept
{
    resume(DSK_FORWARD(cont), get_resumer(ctx), DSK_FORWARD(args)...);
}


constexpr void add_cleanup(_async_ctx_ auto&& ctx, _async_op_ auto&& op)
{
    if constexpr(requires{ ctx.add_cleanup(DSK_FORWARD(op)); })
    {
        ctx.add_cleanup(DSK_FORWARD(op));
    }
    else
    {
        static_assert(false, "unsupported method");
    }
}

constexpr bool set_canceled_if_stop_requested(auto& errOrEc, _async_ctx_ auto&& ctx) noexcept
{
    if(stop_requested(ctx))
    {
        errOrEc = errc::canceled;
        return true;
    }

    return false;
}

constexpr bool set_canceled_if_stop_requested(auto& errOrEc, _stop_source_ auto&& s) noexcept
{
    if(s.stop_requested())
    {
        errOrEc = errc::canceled;
        return true;
    }

    return false;
}


[[nodiscard]] constexpr bool is_immediate(_async_op_ auto const& op) noexcept
{
    if constexpr(requires{ op.is_immediate(); }) return op.is_immediate();
    else                                         return false;
}


[[nodiscard]] constexpr decltype(auto) initiate(_async_op_ auto& op, _continuation_ auto&& cont)
{
    if constexpr(requires{ op.initiate(DSK_FORWARD(cont)); })
    {
        return op.initiate(DSK_FORWARD(cont));
    }
    else if constexpr(requires{ op.initiate(); })
    {
        return op.initiate();
    }
    else
    {
        if constexpr(_no_cvref_same_as_<decltype(cont), continuation>)
        {
            return cont.tail_resume();
        }
        else if constexpr(_coro_handle_<decltype(cont)>)
        {
            DSK_ASSERT(cont);
            return DSK_FORWARD(cont);
        }
        else
        {
            DSK_FORWARD(cont)();
            // return std::noop_coroutine();
        }
    }
}

[[nodiscard]] constexpr decltype(auto) initiate(_async_op_ auto& op, _async_ctx_ auto&& ctx, _continuation_ auto&& cont)
{
    if constexpr(requires{ op.initiate(DSK_FORWARD(ctx), DSK_FORWARD(cont)); })
    {
        return op.initiate(DSK_FORWARD(ctx), DSK_FORWARD(cont));
    }
    else if constexpr(requires{ op.initiate(DSK_FORWARD(ctx)); })
    {
        return op.initiate(DSK_FORWARD(ctx));
    }
    else
    {
        return initiate(op, DSK_FORWARD(cont));
    }
}

// if _async_op_'s initiate() is not called by await_suspend(), i.e. _async_op_ is not co_await'ed,
// you can use this to simulate await_suspend()'s behavior.
// NOTE: cont may be resumed inside manual_initiate(), which may destroy related resources,
//       so don't use these resources after manual_initiate().
void manual_initiate(_async_op_ auto& op, _async_ctx_ auto&& ctx, _continuation_ auto&& cont)
{
    if(is_immediate(op))
    {
        resume(mut_move(cont), ctx);
        return;
    }

    using init_ret = decltype(initiate(op, DSK_FORWARD(ctx), DSK_FORWARD(cont)));

    if constexpr(_coro_handle_<init_ret>)
    {
        initiate(op, DSK_FORWARD(ctx), DSK_FORWARD(cont)).resume(); // should never return null _coro_handle_
    }
    else if constexpr(_same_as_<init_ret, bool>)
    {
        // if initiate() returns false, cont should not be moved.
        if(! initiate(op, DSK_FORWARD(ctx), DSK_FORWARD(cont)))
        {
            resume(mut_move(cont), ctx);
        }
    }
    else
    {
        static_assert(_void_<init_ret>);
        initiate(op, DSK_FORWARD(ctx), DSK_FORWARD(cont));
    }
}

[[nodiscard]] constexpr bool is_failed(_async_op_ auto const& op) noexcept
{
    if constexpr(requires{ op.is_failed(); }) return op.is_failed();
    else                                      return false;
}


// should be called only once on op
template<void_result_e VE = void_as_void>
[[nodiscard]] constexpr decltype(auto) take_result(_async_op_ auto&& op)
{
    if constexpr(requires{ op.take_result(); })
    {
        if constexpr(_void_<decltype(op.take_result())>) return return_void<VE>();
        else                                             return op.take_result();
    }
    else
    {
        return return_void<VE>();
    }
}

// should be called only once on op
template<void_result_e VE = void_as_void>
[[nodiscard]] constexpr decltype(auto) take_result_value(_async_op_ auto&& op)
{
    if constexpr(_result_<decltype(take_result<VE>(DSK_FORWARD(op)))>)
    {
        return take_value<VE>(take_result<VE>(DSK_FORWARD(op)));
    }
    else
    {
        return take_result<VE>(DSK_FORWARD(op));
    }
}

enum take_cat_e
{
          result_cat = 1,
    result_value_cat
};

template<take_cat_e Cat, void_result_e VE = void_as_void>
[[nodiscard]] constexpr decltype(auto) take(_async_op_ auto&& op)
{
         if constexpr(Cat ==       result_cat) return       take_result<VE>(DSK_FORWARD(op));
    else if constexpr(Cat == result_value_cat) return take_result_value<VE>(DSK_FORWARD(op));
    else
        static_assert(false, "unknown category");
}


constexpr auto make_async_ctx() noexcept
{
    return null_async_ctx();
}

constexpr auto&& make_async_ctx(_async_ctx_ auto&& b) noexcept
{
    return DSK_FORWARD(b);
}


template<class StopSource, class Resumer>
struct stop_source_resumer_async_ctx
{
    using is_async_ctx = void;

    DSK_DEF_MEMBER_IF(! _blank_<StopSource>, StopSource) s;
    DSK_DEF_MEMBER_IF(! _blank_<Resumer>, Resumer) r;

    auto& get_stop_source(this auto&& self) noexcept requires(! _blank_<StopSource>) { return unwrap_ref(self.s); }
    auto& get_resumer(this auto&& self) noexcept requires(! _blank_<Resumer>) { return self.r; }
};

auto make_async_ctx(_stop_source_or_ref_wrap_ auto&& s, _scheduler_or_resumer_ auto&& sr)
{
    return make_templated<stop_source_resumer_async_ctx>(DSK_FORWARD(s), get_resumer(DSK_FORWARD(sr)));
}

auto make_async_ctx(_scheduler_or_resumer_ auto&& sr, _stop_source_or_ref_wrap_ auto&& s)
{
    return make_async_ctx(DSK_FORWARD(s), DSK_FORWARD(sr));
}

auto make_async_ctx(_stop_source_or_ref_wrap_ auto&& s)
{
    return make_templated<stop_source_resumer_async_ctx>(DSK_FORWARD(s), blank_c);
}

auto make_async_ctx(_scheduler_or_resumer_ auto&& sr)
{
    return make_templated<stop_source_resumer_async_ctx>(blank_c, get_resumer(DSK_FORWARD(sr)));
}


template<_async_ctx_ Base, _stop_source_or_ref_wrap_ StopSource>
struct override_stop_source_async_ctx : Base
{
    StopSource s;

    auto& get_stop_source(this auto&& self) noexcept { return unwrap_ref(self.s); }
};

auto make_async_ctx(_async_ctx_ auto&& base, _stop_source_or_ref_wrap_ auto&& s)
{
    return make_templated<override_stop_source_async_ctx>(DSK_FORWARD(base), DSK_FORWARD(s));
}


template<_async_ctx_ Base, _resumer_ Resumer>
struct override_resumer_async_ctx : Base
{
    DSK_NO_UNIQUE_ADDR Resumer r;

    auto& get_resumer(this auto&& self) noexcept { return self.r; }
};

auto make_async_ctx(_async_ctx_ auto&& base, _scheduler_or_resumer_ auto&& sr)
{
    return make_templated<override_resumer_async_ctx>(DSK_FORWARD(base), get_resumer(DSK_FORWARD(sr)));
}


template<_async_ctx_ Base, _stop_source_or_ref_wrap_ StopSource, _resumer_ Resumer>
struct override_stop_source_resumer_async_ctx : Base
{
    StopSource s;
    DSK_NO_UNIQUE_ADDR Resumer r;

    auto& get_stop_source(this auto&& self) noexcept { return unwrap_ref(self.s); }
    auto& get_resumer(this auto&& self) noexcept { return self.r; }
};

auto make_async_ctx(_async_ctx_ auto&& base, _stop_source_or_ref_wrap_ auto&& s, _scheduler_or_resumer_ auto&& sr)
{
    return make_templated<override_stop_source_resumer_async_ctx>(DSK_FORWARD(base), DSK_FORWARD(s), get_resumer(DSK_FORWARD(sr)));
}


template<bool Cond>
decltype(auto) make_async_ctx_if(_async_ctx_ auto&& base, auto&&... args)
{
    if constexpr(Cond) return make_async_ctx(DSK_FORWARD(base), DSK_FORWARD(args)...);
    else               return DSK_FORWARD(base);
}


template<_async_op_ Op, class Overrider>
struct override_ctx_async_op : Op
{
    DSK_NO_UNIQUE_ADDR Overrider overrider;

    decltype(auto) initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        return dsk::initiate(static_cast<Op&>(*this), overrider(DSK_FORWARD(ctx)), DSK_FORWARD(cont));
    }
};

auto override_ctx(_async_op_ auto&& op, std::invocable<dummy_async_ctx> auto&& overrider)
{
    return make_templated<override_ctx_async_op>(DSK_FORWARD(op), DSK_FORWARD(overrider));
}

auto override_ctx(_async_op_ auto&& op, auto&&... args) requires(sizeof...(args) > 0)
{
    return override_ctx
    (
        DSK_FORWARD(op),
        [...args = get_resumer_or_fwd(DSK_FORWARD(args))](_async_ctx_ auto&& ctx) mutable
        {
            return make_async_ctx(DSK_FORWARD(ctx), mut_move(args)...);
        }
    );
}

template<bool Cond>
decltype(auto) override_ctx_if(_async_op_ auto&& op, auto&&... args)
{
    if constexpr(Cond) return override_ctx(DSK_FORWARD(op), DSK_FORWARD(args)...);
    else               return DSK_FORWARD(op);
}


auto set_stop_source(_stop_source_or_ref_wrap_ auto&& s, _async_op_ auto&& op)
{
    return override_ctx(DSK_FORWARD(op), DSK_FORWARD(s));
}

auto set_resumer(_scheduler_or_resumer_ auto&& sr, _async_op_ auto&& op)
{
    return override_ctx(DSK_FORWARD(op), DSK_FORWARD(sr));
}


template<_async_ctx_ B>
struct inline_resume_async_ctx : B
{
    explicit inline_resume_async_ctx(auto&& b) : B(DSK_FORWARD(b)) {}
    void get_resumer() = delete;
};

template<class B>
inline_resume_async_ctx(B) -> inline_resume_async_ctx<B>;


template<_async_ctx_ B>
struct no_cancel_async_ctx : B
{
    explicit no_cancel_async_ctx(auto&& b) : B(DSK_FORWARD(b)) {}
    void get_stop_source() = delete;
};

template<class B>
no_cancel_async_ctx(B) -> no_cancel_async_ctx<B>;


template<_async_ctx_ B>
struct no_cleanup_async_ctx : B
{
    explicit no_cleanup_async_ctx(auto&& b) : B(DSK_FORWARD(b)) {}
    void add_cleanup() = delete;
};

template<class B>
no_cleanup_async_ctx(B) -> no_cleanup_async_ctx<B>;


template<_async_ctx_ B>
struct no_cancel_cleanup_async_ctx : B
{
    explicit no_cancel_cleanup_async_ctx(auto&& b) : B(DSK_FORWARD(b)) {}
    void get_stop_source() = delete;
    void add_cleanup() = delete;
};

template<class B>
no_cancel_cleanup_async_ctx(B) -> no_cancel_cleanup_async_ctx<B>;



template<class F>
struct sync_async_op
{
    static decltype(auto) _invoke(auto&& f, _async_ctx_ auto&& ctx)
    {
        if constexpr(requires{ f(ctx); }) return f(ctx);
        else                              return f();
    }

    using result_type = decltype(_invoke(std::declval<F&>(), dummy_async_ctx()));

    DSK_NO_UNIQUE_ADDR F _f;
    DSK_DEF_MEMBER_IF(! _void_<result_type>, optional<result_type>) _r;

    constexpr explicit sync_async_op(auto&& f) : _f(DSK_FORWARD(f)) {}

    using is_async_op = void;

    constexpr bool initiate(_async_ctx_ auto&& ctx)
    {
        if constexpr(_void_<result_type>) _invoke(_f, ctx);
        else                              _r.emplace(_invoke(_f, ctx));

        return false; // immediate resume
    }

    constexpr bool is_failed() const noexcept requires(_errorable_<result_type>)
    {
        return has_err(*_r);
    }

    constexpr result_type take_result() requires(! _void_<result_type>)
    {
        return *mut_move(_r);
    }
};

template<class F>
sync_async_op(F) -> sync_async_op<F>;


} // namespace dsk
