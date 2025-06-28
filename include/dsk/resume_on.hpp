#pragma once

#include <dsk/config.hpp>
#include <dsk/resumer.hpp>
#include <dsk/scheduler.hpp>
#include <dsk/async_op.hpp>
#include <dsk/continuation.hpp>


namespace dsk{


template<class Op, class Resumer>
struct resume_op_cont_op : Op
{
    DSK_NO_UNIQUE_ADDR Resumer _resumer;

    decltype(auto) initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        return dsk::initiate(static_cast<Op&>(*this), DSK_FORWARD(ctx),
                             bind_resumer(DSK_FORWARD(cont), mut_move(_resumer)));
    }
};

// Resumes the continuation of 'op' on 'sr'.
auto resume_on(_scheduler_or_resumer_ auto&& sr, _async_op_ auto&& op)
{
    return make_templated<resume_op_cont_op>(DSK_FORWARD(op), get_resumer(DSK_FORWARD(sr)));
}


template<class Resumer>
struct resume_coro_op
{
    DSK_NO_UNIQUE_ADDR Resumer _resumer;
                       errc    _err{};

    using is_async_op = void;

    bool initiate(_async_ctx_ auto&& ctx, _coro_handle_ auto&& coro)
    {
        if(set_canceled_if_stop_requested(_err, ctx))
        {
            return false;
        }

        _resumer.post(DSK_FORWARD(coro));
        return true;
    }

    bool is_failed() const noexcept { return has_err(_err); }
    errc take_result() noexcept { return _err; }
};

// Resumes current coroutine on 'sr'.
auto resume_on(_scheduler_or_resumer_ auto&& sr)
{
    return make_templated<resume_coro_op>(get_resumer(DSK_FORWARD(sr)));
}


} // namespace dsk
