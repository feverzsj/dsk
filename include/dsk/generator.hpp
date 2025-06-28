#pragma once

#include <dsk/task.hpp>


namespace dsk{


// Async cleanup can be used, even if the generator didn't finish,
// as the cleanup stack op is added to the parent cleanup stack.
// So, cleanup scope must matches the generator's lifetime.
// 
// NOTE: you cannot move a started generator to another _async_ctx_.
template<
    class T = void,
    class E = error_code,
    class Alloc = DSK_DEFAULT_ALLOCATOR<void>
>
class generator : public coro_host_base<generator<T, E, Alloc>, T, E, Alloc, true>
{
    using base = coro_host_base<generator<T, E, Alloc>, T, E, Alloc, true>;

    struct parent_cleanup_op
    {
        generator& g;
        std::optional<decltype(g.promise().cleanup())> op;
        
        using is_async_op = void;

        bool is_immediate() const noexcept
        {
            return ! g.valid();
        }

        decltype(auto) initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
        {
            op.emplace(g.promise().cleanup());
            return dsk::initiate(*op, DSK_FORWARD(ctx), DSK_FORWARD(cont));
        }

        bool is_failed() const noexcept
        {
            return op ? dsk::is_failed(*op) : false;
        }

        auto take_result()
        {
            return op ? dsk::take_result(*op) : DSK_NO_CVREF_T(dsk::take_result(*op))();
        }
    };

public:
    using base::base;

    struct next_async_op
    {
        generator& g;

        using is_async_op = void;
        
        bool is_immediate() const noexcept
        {
            return ! g.valid();
        }

        auto initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont) noexcept
        {
            if(! g.continuation()) // first co_await g.next(), suppose always same (s,c)
            {
                g.set_stop_source(get_stop_source(ctx));
                g.set_continuation(DSK_FORWARD(cont));
                add_cleanup(ctx, parent_cleanup_op(g));
            }

            return g.coro_handle();
        }

        bool is_failed() const noexcept
        {
            return g.is_failed();
        }

        decltype(auto) take_result()
        {
            return g.take_result();
        }
    };

    auto next()
    {
        return next_async_op{*this};
    }
};


} // namespace dsk
