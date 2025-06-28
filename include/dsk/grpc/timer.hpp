#pragma once

#include <dsk/async_op.hpp>
#include <dsk/grpc/err.hpp>
#include <dsk/grpc/cq.hpp>
#include <grpcpp/alarm.h>


namespace dsk{


template<class Cq = grpc_cq>
class grpc_timer_t
{
public:
    using clock = std::chrono::system_clock;
    using duration = clock::duration;
    using time_point = clock::time_point;

private:
    Cq& _cq;
    grpc::Alarm _a;

    struct wait_op
    {
        grpc_timer_t&          _d;
        time_point             _t;
        errc                   _err{};
        continuation           _cont;
        optional_stop_callback _scb; // must be last one defined

        using is_async_op = void;

        bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
        {
            DSK_ASSERT(! has_err(_err));

            if(stop_requested(ctx))
            {
                _err = errc::canceled;
                return false;
            }

            _cont = DSK_FORWARD(cont);

            if(stop_possible(ctx))
            {
                _scb.emplace(get_stop_token(ctx), [this]()
                {
                    _d._a.Cancel();
                });
            }
            
            _d._cq.post
            (
                [this](void* tag)
                {
                    _d._a.Set(_d._cq.cq(), _t, tag);
                },
                [this, ctx](bool ok) mutable
                {
                    if(! ok)
                    {
                        _err = errc::canceled;
                    }

                    resume(mut_move(_cont), ctx);
                }
            );

            return true;
        }

        bool is_failed() const noexcept { return has_err(_err); }
        auto take_result() noexcept { return make_expected_if_no(_err); }
    };

public:
    explicit grpc_timer_t(Cq& cq)
        : _cq(cq)
    {}

    auto wake_at(time_point const& t)
    {
        return wait_op(*this, t);
    }

    auto sleep_for(duration const& d)
    {
        return wake_at(clock::now() + d);
    }
};


using grpc_timer = grpc_timer_t<>;


} // namespace dsk
