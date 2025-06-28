#pragma once

#include <dsk/config.hpp>
#include <dsk/expected.hpp>
#include <dsk/optional.hpp>
#include <dsk/awaitables.hpp>
#include <dsk/any_resumer.hpp>
#include <dsk/async_op.hpp>
#include <dsk/async_op_group.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/util/allocator.hpp>
#include <memory>
#include <functional>
#include <stop_token>


namespace dsk{


template<class Promise = void>
using unique_coro_handle = unique_handle<
    std::coroutine_handle<Promise>,
    [](std::coroutine_handle<Promise> c){ c.destroy(); },
    nullptr
>;


struct [[nodiscard]]        get_promise_t{};
struct [[nodiscard]]    get_stop_source_t{};
struct [[nodiscard]]     get_stop_token_t{};
struct [[nodiscard]]      stop_possible_t{};
struct [[nodiscard]]     stop_requested_t{};
struct [[nodiscard]]       request_stop_t{};
struct [[nodiscard]] make_cleanup_scope_t{};
struct [[nodiscard]]      get_async_ctx_t{};
struct [[nodiscard]]        get_resumer_t{};

constexpr        get_promise_t        get_promise() noexcept { return {}; }
constexpr     get_stop_token_t     get_stop_token() noexcept { return {}; }
constexpr    get_stop_source_t    get_stop_source() noexcept { return {}; }
constexpr      stop_possible_t      stop_possible() noexcept { return {}; }
constexpr     stop_requested_t     stop_requested() noexcept { return {}; }
constexpr       request_stop_t       request_stop() noexcept { return {}; }
constexpr make_cleanup_scope_t make_cleanup_scope() noexcept { return {}; }
constexpr      get_async_ctx_t      get_async_ctx() noexcept { return {}; }
constexpr        get_resumer_t        get_resumer() noexcept { return {}; }


struct [[nodiscard]] set_stop_source_t
{
    std::stop_source const& s;
};

auto set_stop_source(std::stop_source const& s) noexcept
{
    return set_stop_source_t(s);
}


struct [[nodiscard]] set_resumer_t
{
    any_resumer const& r;
};

auto set_resumer(any_resumer const& r) noexcept
{
    return set_resumer_t(r);
}


template<class F>
struct [[nodiscard]] create_stop_callback_t
{
    F&& f;
};

auto create_stop_callback(auto&& f) noexcept
{
    return create_stop_callback_t<decltype(f)>{DSK_FORWARD(f)};
}


template<class Op>
struct [[nodiscard]] add_cleanup_t
{
    Op&& op;
};

auto add_cleanup(_async_op_ auto&& op) noexcept
{
    return add_cleanup_t<decltype(op)>{DSK_FORWARD(op)};
}


template<class Op>
struct [[nodiscard]] add_parent_cleanup_t
{
    Op&& op;
};

auto add_parent_cleanup(_async_op_ auto&& op) noexcept
{
    return add_parent_cleanup_t<decltype(op)>{DSK_FORWARD(op)};
}


template<class E>
struct [[nodiscard]] coro_throw_t
{
    E&& err;
};

auto coro_throw(auto&& e) noexcept { return coro_throw_t<decltype(e)>{DSK_FORWARD(e)}; }


struct [[nodiscard]] co_return_dummy{};

template<class ArgTuple>
struct [[nodiscard]] coro_return_t
{
    ArgTuple args;
};

auto coro_return(auto&&... args) noexcept { return coro_return_t{fwd_as_tuple(DSK_FORWARD(args)...)}; }


#define DSK_WAIT co_await
#define DSK_YIELD co_yield
#define DSK_TRY co_await co_await
#define DSK_THROW(...) co_await coro_throw(__VA_ARGS__)
// DSK_TRY_SYNC _errorable_/_result_
#define DSK_TRY_SYNC co_await

#define DSK_CATH_ANY_CPP_EXCEPTION()                                                    \
    catch(boost::system::system_error const& e) { DSK_THROW(e.code()); }                \
    catch(          std::system_error const& e) { DSK_THROW(e.code()); }                \
    catch(                                 ...) { DSK_THROW(errc::unknown_exception); } \
/**/

// NOTE: "Automatic move from local variables and parameters" is not supported.
//       Use DSK_RETURN(mut_move(...))/DSK_RETURN_MOVED(...), if move is preferred.
#define DSK_RETURN(...)                    \
    do{                                    \
        co_await coro_return(__VA_ARGS__); \
        DSK_UNREACHABLE();                 \
        co_return co_return_dummy();       \
    }                                      \
    while(false)                           \
/**/

#define DSK_RETURN_MOVED(...) DSK_RETURN(mut_move(__VA_ARGS__))

#define DSK_TRY_RETURN(...) DSK_RETURN(DSK_TRY __VA_ARGS__)
#define DSK_TRY_SYNC_RETURN(...) DSK_RETURN(DSK_TRY_SYNC __VA_ARGS__)



// NOTE: By default, cleanup op will be added to the closest cleanup guard.
//       You can explicitly cleanup the same guard multiple times via `DSK_TRY your_cleanup_guard.until_all_done()`.

#define _DSK_CLEANUP_SCOPE_NAME __dsk_cleanup_scope__

#define DSK_CLEANUP_SCOPE_ADD(...) _DSK_CLEANUP_SCOPE_NAME.add(__VA_ARGS__)
#define DSK_CLEANUP_SCOPE_CLEAN_UP()  DSK_TRY _DSK_CLEANUP_SCOPE_NAME.until_all_done()

#define DSK_CLEANUP_SCOPE_BEGIN() auto _DSK_CLEANUP_SCOPE_NAME = co_await make_cleanup_scope()
#define DSK_CLEANUP_SCOPE_END() DSK_CLEANUP_SCOPE_CLEAN_UP()

#define DSK_CLEANUP_SCOPE(...)     \
    {                              \
        DSK_CLEANUP_SCOPE_BEGIN(); \
        __VA_ARGS__                \
        DSK_CLEANUP_SCOPE_END();   \
    }                              \
/**/


class coro_promise_base
{
protected:
    struct coro_async_ctx
    {
        using is_async_ctx = void;
        
        coro_promise_base& p;

        auto& promise() noexcept { return p; }

        auto& get_stop_source(this auto&& self) noexcept { return self.p.get_stop_source(); }
        auto& get_resumer(this auto&& self) noexcept { return self.p.get_resumer(); }
        void  add_cleanup(_async_op_ auto&& op) { p.add_cleanup(DSK_FORWARD(op)); }
    };

    // cleanup is NOT cancellable and no more cleanup op should be added.
    struct coro_cleanup_async_ctx
    {
        using is_async_ctx = void;

        coro_promise_base& p;

        auto& promise() noexcept { return p; }
        auto& get_resumer(this auto&& self) noexcept { return self.p.get_resumer(); }
    };

    coro_promise_base* _parent = nullptr;
    continuation       _cont;
    std::stop_source   _ss{std::nostopstate};
    any_resumer        _resumer; // just for async ctx, not this->_cont.

    lazy_async_op_group_stack
    <
        coro_cleanup_async_ctx,
        null_op_t,
        errc::one_or_more_cleanup_ops_failed
    >
    _cleanupGpStack{coro_cleanup_async_ctx(*this)};

public:
    // coroutine has many exist points, but cleanup can be done only at those awaitable suspend points.
    ~coro_promise_base()
    {
        _cleanupGpStack.pop_untill_non_empty();
        DSK_RELEASE_ASSERT(_cleanupGpStack.empty());
    }

    auto get_async_ctx() noexcept
    {
        return coro_async_ctx{*this};
    }

    void  set_stop_source(_stop_source_ auto&& s) noexcept { _ss = DSK_FORWARD(s); }
    auto& get_stop_source(this auto& self) noexcept { return self._ss; }
    auto   get_stop_token() const noexcept { return _ss.     get_token(); }
    bool   stop_requested() const noexcept { return _ss.stop_requested(); }
    bool     request_stop()       noexcept { return _ss.  request_stop(); }

    void  set_resumer(any_resumer const& r) noexcept { _resumer = r; }
    auto& get_resumer(this auto& self) noexcept { return self._resumer; }

    auto& continuation() const noexcept { return _cont; };
    void set_continuation(_continuation_ auto&& c)
    {
        DSK_ASSERT(! _cont);
        _cont = DSK_FORWARD(c);
    }

    void set_parent(coro_promise_base& p)
    {
        _parent = std::addressof(p);
    }

    void add_parent_cleanup(_async_op_ auto&& op)
    {
        DSK_ASSERT(_parent);
        _parent->add_cleanup(DSK_FORWARD(op));
    }

    void add_cleanup(_async_op_ auto&& op)
    {
        if(_cleanupGpStack.empty())
        {
            _cleanupGpStack.push();
        }

        _cleanupGpStack.top().add(DSK_FORWARD(op));
    }

    auto cleanup() noexcept
    {
        return _cleanupGpStack.until_all_done();
    }
};


template<class Host, class Alloc, bool IsGen>
class coro_promise : public coro_promise_base
{
    // local variables defined in coroutine should not be over-aligned.
    // ref: https://stackoverflow.com/questions/66546906/is-it-defined-behavior-to-place-exotically-aligned-objects-in-the-coroutine-stat
    constexpr static size_t default_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__; // alignof(std::max_align_t);

    struct alignas(default_align) aligned_t
    {
        unsigned char pad[default_align];
    };

    static_assert(alignof(aligned_t) == sizeof(aligned_t));

    constexpr static size_t aligned_count(size_t s) noexcept
    {
        return (s + sizeof(aligned_t) - 1) / sizeof(aligned_t);
    }

public:
    using allocator_type = rebind_alloc<Alloc, aligned_t>;

    static_assert(is_stateless_allocator_v<allocator_type>);

    static auto get_allocator() noexcept { return allocator_type(); }

    static void* operator new(size_t s) noexcept
    {
        try{
            return allocator_type().allocate(aligned_count(s));
        }
        catch(...)
        {
            return nullptr;
        }
    }

    static void operator delete(void* p, size_t s)
    {
        allocator_type().deallocate(static_cast<aligned_t*>(p), aligned_count(s));
    }

private:
    Host* _ho = nullptr;

public:
    Host& host() noexcept { DSK_ASSERT(_ho); return *_ho; }
    void set_host(Host* ho) { _ho = ho; }

    auto coro_handle() noexcept
    {
        return std::coroutine_handle<coro_promise>::from_promise(*this);
    }

    // if error raises in cleanup, it will overwrite exisitng result;
    std::coroutine_handle<> initiate_cleanup(auto& cleanupOp)
    {
        bool initiated = initiate(cleanupOp, this->get_async_ctx(), [&]() mutable
        {
            if(is_failed(cleanupOp))
            {
                _ho->emplace_err(get_err(take_result(cleanupOp)));
            }

            auto cont = mut_move(_cont);

            _ho->destroy_coro();

            cont.resume();
        });

        if(initiated)
        {
            return std::noop_coroutine();
        }

        if(is_failed(cleanupOp))
        {
            _ho->emplace_err(get_err(take_result(cleanupOp)));
        }

        auto cont = mut_move(_cont);

        _ho->destroy_coro();

        return cont.tail_resume(); // immediate resume continuation if any
    }

    auto initiate_throw(auto&& e, auto& cleanupOp)
    {
        _ho->emplace_err(DSK_FORWARD(e));
        return initiate_cleanup(cleanupOp);
    }

// promise concept

    auto get_return_object() noexcept { return Host(coro_handle()); }

    static auto get_return_object_on_allocation_failure() noexcept
    {
        Host ho;
        ho.emplace_err(errc::allocation_failed);
        return ho;
    }

    auto initial_suspend() noexcept { return std::suspend_always(); }

    auto final_suspend() noexcept
    {
        DSK_ABORT("coroutine should not reach final_suspend(). Did you forget DSK_RETURN()?");
        return std::suspend_always();
    }

    void unhandled_exception() noexcept
    {
        DSK_ABORT("expcetion should not cross coroutine boundary");
    }

    void return_value(co_return_dummy) noexcept
    {
        DSK_ABORT("DSK_RETURN(...) should be used to return from coroutine");
    }

    // return_void() not defined, so compiler would warn about co_return not used,
    // then DSK_RETURN() should be used.

    auto yield_value(auto&& v) requires(IsGen)
    {
        _ho->emplace_val(DSK_FORWARD(v));

        return suspend_by_invoke([this]()
        {
            return _cont.tail_resume();
        });
    }

// await_transform

    auto await_transform(get_promise_t) noexcept
    {
        return resume_by_forward(*this);
    }

    auto await_transform(get_stop_source_t) noexcept
    {
        return resume_by_forward(_ss);
    }

    auto await_transform(set_stop_source_t&& s) noexcept
    {
        return resume_by_invoke([&](){ set_stop_source(s.s); });
    }

    auto await_transform(get_stop_token_t) noexcept
    {
        return resume_by_invoke([&](){ return _ss.get_token(); });
    }

    auto await_transform(stop_possible_t) noexcept
    {
        return resume_by_invoke([&](){ return _ss.stop_possible(); });
    }

    auto await_transform(stop_requested_t) noexcept
    {
        return resume_by_invoke([&](){ return _ss.stop_requested(); });
    }

    auto await_transform(request_stop_t) noexcept
    {
        return resume_by_invoke([&](){ return _ss.request_stop(); });
    }

    template<class F>
    auto await_transform(create_stop_callback_t<F>&& t) noexcept
    {
        return resume_by_invoke([&](){ return std::stop_callback(_ss.get_token(), DSK_FORWARD(t.f)); });
    }

    template<class F>
    auto await_transform(invoke_with_async_ctx<F>&& t) noexcept
    {
        return resume_by_invoke([&]() -> decltype(auto) { return t.f(this->get_async_ctx()); });
    }

    auto await_transform(make_cleanup_scope_t) noexcept
    {
        return resume_by_invoke([&]() { return _cleanupGpStack.make_push_pop_guard(); });
    }

    template<class Op>
    auto await_transform(add_cleanup_t<Op>&& t) noexcept
    {
        return resume_by_invoke([&](){ return this->add_cleanup(DSK_FORWARD(t.op)); });
    }

    template<class Op>
    auto await_transform(add_parent_cleanup_t<Op>&& t) noexcept
    {
        return resume_by_invoke([&](){ return this->add_parent_cleanup(DSK_FORWARD(t.op)); });
    }

    auto await_transform(get_async_ctx_t) noexcept
    {
        return resume_by_invoke([&](){ return this->get_async_ctx(); });
    }

    auto await_transform(get_resumer_t) noexcept
    {
        return resume_by_invoke([&](){ return get_resumer(); });
    }

    auto await_transform(set_resumer_t&& r) noexcept
    {
        return resume_by_invoke([&](){ set_resumer(r.r); });
    }

    // DSK_THROW(err)
    template<class E>
    auto await_transform(coro_throw_t<E>&& t) noexcept
    {
        return suspend_by_invoke([&, cleanupOp = cleanup()]() mutable
        {
            return initiate_throw(DSK_FORWARD(t.err), cleanupOp);
        });
    }

    // DSK_RETURN(...)
    template<class ArgTuple>
    auto await_transform(coro_return_t<ArgTuple>&& t) noexcept
    {
        return suspend_by_invoke([&, cleanupOp = cleanup()]() mutable
        {
            return apply_elms(t.args, [&](auto&&... args)
            {
                if constexpr(IsGen)
                {
                    static_assert(sizeof...(args) == 0, "generator shouldn't return value");
                }

                _ho->emplace_val(DSK_FORWARD(args)...); // for generator, a default constructed optional, which is null, to end next() loop.

                return initiate_cleanup(cleanupOp);
            });
        });
    }

    auto await_transform(_errorable_ auto&& t) noexcept
    {
        struct errorable_awaiter
        {
            decltype(t)&& t; // use reference, as temporary lifetime should last until co_await expression returns.
            decltype(_cleanupGpStack.until_all_done()) cleanupOp;

            constexpr bool await_ready() noexcept
            {
                return ! has_err(t);
            }

            constexpr auto await_suspend(std::coroutine_handle<coro_promise> h) noexcept
            {
                return h.promise().initiate_throw(get_err(DSK_FORWARD(t)), cleanupOp);
            }

            constexpr decltype(auto) await_resume()
            {
                if constexpr(_result_<decltype(t)>)
                {
                    return take_value(DSK_FORWARD(t));
                }
            }
        };

        return errorable_awaiter(DSK_FORWARD(t), cleanup());
    }

    auto await_transform(_async_op_ auto&& op) noexcept
    {
        struct async_op_awaiter
        {
            decltype(op)& op; // use reference, as temporary lifetime should last until co_await expression returns.

            constexpr bool await_ready() const noexcept
            {
                return is_immediate(op);
            }

            constexpr auto await_suspend(std::coroutine_handle<coro_promise> h) noexcept
            {
                return initiate(op, h.promise().get_async_ctx(), h);
            }

            constexpr decltype(auto) await_resume()
            {
                return take_result(op);
            }
        };

        return async_op_awaiter(op);
    }
};


template<class Host, class T, class E, class Alloc, bool IsGen = false>
class coro_host_base
{
public:
    using promise_type = coro_promise<Host, Alloc, IsGen>;
    using allocator_type = promise_type::allocator_type;

protected:
    friend promise_type;
    using result_type = expected<std::conditional_t<IsGen, optional<T>, T>, E>;

    unique_coro_handle<promise_type> _h;
    optional<result_type>            _r;

    bool has_result() const noexcept
    {
        return _r.has_value();
    }

    void emplace_val(auto&&... args)
    {
        DSK_ASSERT(! _r);
        _r.emplace(expect, DSK_FORWARD(args)...);
    }

    void emplace_err(auto&& e)
    {
        //DSK_ASSERT(! _r); cleanup may re-emplace
        _r.emplace(unexpect, DSK_FORWARD(e));
    }

    // only valid before take_result() being called
    bool is_failed() const noexcept
    {
        DSK_ASSERT(_r);
        return has_err(*_r);
    }

    result_type take_result()
    {
        DSK_ASSERT(_r);

        if constexpr(IsGen)
        {
            auto t = mut_move(_r);
            _r.reset();
            return *mut_move(t);
        }
        else
        {
            return *mut_move(_r);
        }
    }

    void destroy_coro()
    {
        _h.reset();
    }

public:
    coro_host_base() = default;

    explicit coro_host_base(std::coroutine_handle<promise_type> c) noexcept
        : _h(c)
    {
        if(_h)
        {
            promise().set_host(static_cast<Host*>(this));
        }
    }

    coro_host_base(coro_host_base&& r) noexcept
        : _h(mut_move(r._h)), _r(mut_move(r._r))
    {
        if(_h)
        {
            promise().set_host(static_cast<Host*>(this));
        }
    }

    coro_host_base& operator=(coro_host_base&& r) noexcept
    {
        _h = mut_move(r._h);
        _r = mut_move(r._r);

        if(_h)
        {
            promise().set_host(static_cast<Host*>(this));
        }
    }

    // !valid() could mean finsined or default constructed.
    bool valid() const noexcept { return _h.valid(); }
    explicit operator bool() const noexcept { return valid(); }

    // !coro_handle() could mean finsined or default constructed.
    auto coro_handle() const noexcept { return _h.get(); }

    // NOTE: not use explicit object parameter, as coroutine_handle::promise doesn't propagate constness.
    auto      & promise()       noexcept { return _h->promise(); }
    auto const& promise() const noexcept { return _h->promise(); }

    auto get_async_ctx() noexcept { return promise().get_async_ctx(); }

    void  set_stop_source(_stop_source_ auto&& s) noexcept { promise().set_stop_source(DSK_FORWARD(s)); }
    auto& get_stop_source(this auto& self) noexcept { return self.promise().get_stop_source(); }
    auto   get_stop_token() const noexcept { return promise().get_stop_token(); }
    bool   stop_requested() const noexcept { return promise().stop_requested(); }
    bool     request_stop()       noexcept { return promise().  request_stop(); }

    void  set_resumer(any_resumer const& r) noexcept { promise().set_resumer(r); }
    auto& get_resumer(this auto& self) noexcept { return self.promise().get_resumer(); }

    auto& continuation() const noexcept { return promise().continuation(); };
    void set_continuation(_continuation_ auto&& c) { promise().set_continuation(DSK_FORWARD(c)); }

    void set_parent(coro_promise_base& p) noexcept { promise().set_parent(p); }
};


// When you call a coroutine that returns a task, the coroutine
// simply captures any passed parameters and returns execution to the
// caller. Execution of the coroutine body does not start until the
// coroutine is first co_await'ed or explicitly started by start[_xxx]().
template<
    class T = void,
    class E = error_code,
    class Alloc = DSK_DEFAULT_ALLOCATOR<void>
>
class [[nodiscard]] task : public coro_host_base<task<T, E>, T, E, Alloc>
{
    using base = coro_host_base<task<T, E>, T, E, Alloc>;

public:
    task() = default;

    using base::base;

    using is_async_op = void;

    bool is_immediate() const noexcept
    {
        return ! base::valid();
    }

    auto initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont) noexcept
    {
        base::set_stop_source(get_stop_source(ctx));
        base::set_resumer(get_resumer(ctx));
        base::set_continuation(DSK_FORWARD(cont));

        if constexpr(requires{ ctx.promise(); })
        {
            base::set_parent(ctx.promise());
        }

        return base::coro_handle(); // once co_awaited, immediately resume self.
    }

    using base::is_failed;
    using base::take_result;
};



// Turn _async_op_ into task<T, E, Alloc>.
// Useful as a polymorphic wrapper for _async_op_.
template<
    class T = void,
    class E = error_code,
    class Alloc = DSK_DEFAULT_ALLOCATOR<void>
>
decltype(auto) as_task(_async_op_ auto&& op)
{
    if constexpr(_no_cvref_same_as_<decltype(op), task<T, E, Alloc>>)
    {
        return DSK_FORWARD(op);
    }
    else
    {        
        return [](auto op) -> task<T, E, Alloc>
        {
            constexpr bool errorable = _errorable_<decltype(take_result(op))>;

            if constexpr(_void_<T>)
            {
                if constexpr(errorable) DSK_TRY op;
                else                    DSK_WAIT op;

                DSK_RETURN();
            }
            else
            {
                if constexpr(errorable) DSK_RETURN(DSK_TRY op);
                else                    DSK_RETURN(DSK_WAIT op);
            }
        }(DSK_FORWARD(op));
    }
}


} // namespace dsk
