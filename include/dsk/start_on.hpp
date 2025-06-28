#pragma once

#include <dsk/config.hpp>
#include <dsk/async_op.hpp>
#include <dsk/awaitables.hpp>
#include <dsk/continuation.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/util/allocate_unique.hpp>


namespace dsk{


template<bool OverrideCtx, bool Solely, class Op, class Resumer>
struct start_async_op_on_scheduler_op : Op
{
    DSK_NO_UNIQUE_ADDR Resumer _resumer;

    using is_async_op = void;

    void initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        _resumer.post([this, ctx = DSK_FORWARD(ctx), cont = DSK_FORWARD(cont)]() mutable
        {
            manual_initiate(static_cast<Op&>(*this),
                            make_async_ctx_if<OverrideCtx>(ctx, mut_move(_resumer)),
                            bind_resumer_if<Solely>(mut_move(cont), get_resumer(ctx)));
        });
    }
};

// Returns an _async_op_ that will initiate 'op' on 'sr'.
template<bool OverrideCtx = false, bool Solely = false>
auto start_on(_scheduler_or_resumer_ auto&& sr, _async_op_ auto&& op)
{
    return make_templated_nt2<start_async_op_on_scheduler_op, OverrideCtx, Solely>
    (
        DSK_FORWARD(op), get_resumer(DSK_FORWARD(sr))
    );
}

// Same as start_on(sr, override_ctx(op, sr))
auto run_on(_scheduler_or_resumer_ auto&& sr, _async_op_ auto&& op)
{
    return start_on<true>(DSK_FORWARD(sr), DSK_FORWARD(op));
}

// Same as run_on(sr, resume_on(ctx_resumer, op))
auto solely_run_on(_scheduler_or_resumer_ auto&& sr, _async_op_ auto&& op)
{
    return start_on<true, true>(DSK_FORWARD(sr), DSK_FORWARD(op));
}


} // namespace dsk
