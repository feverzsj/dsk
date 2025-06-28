#pragma once

#include <dsk/async_op.hpp>
#include <dsk/util/latch.hpp>


namespace dsk{


template<
    take_cat_e   Cat = result_cat,
    void_result_e VE = void_as_void
>
[[nodiscard]] decltype(auto) sync_wait(_async_op_ auto&& op, auto&&... ctxArgs)
{
    if(! is_immediate(op))
    {
        latch done(1);

        manual_initiate(op, make_async_ctx(DSK_FORWARD(ctxArgs)...),
                        [&](){ done.count_down(); });

        done.wait();
    }

    return take<Cat, VE>(op);
}


} // namespace dsk
