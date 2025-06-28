#pragma once

#include <dsk/util/ct.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/grpc/cq_pool.hpp>
#include <grpcpp/server_builder.h>
#include <grpcpp/generic/async_generic_service.h>


namespace dsk{


template<_tuple_like_ Services, class Cq = grpc_cq>
class grpc_srv_t
{
    Services                      _services;
    std::unique_ptr<grpc::Server> _srv;
    grpc_cq_pool_t<Cq>            _cqs;

public:
    using allocator_type = typename Cq::allocator_type;

    grpc_srv_t() = default;

    explicit grpc_srv_t(auto&& b,
                        uint32_t nCq = 1,
                        threads_per_cq threadsPerCq = threads_per_cq::defaulted,
                        start_scheduler_e startNow = dont_start_now)
    {
        setup(DSK_FORWARD(b), nCq, threadsPerCq);

        if(startNow)
        {
            start();
        }
    }

    explicit grpc_srv_t(auto&& b, uint32_t nCq, start_scheduler_e startNow = dont_start_now)
        : grpc_srv_t(DSK_FORWARD(b), nCq, threads_per_cq::defaulted, startNow)
    {}

    explicit grpc_srv_t(auto&& b, threads_per_cq threadsPerCq, start_scheduler_e startNow = dont_start_now)
        : grpc_srv_t(DSK_FORWARD(b), 1, threadsPerCq, startNow)
    {}

    explicit grpc_srv_t(auto&& b, start_scheduler_e startNow)
        : grpc_srv_t(DSK_FORWARD(b), 1, threads_per_cq::defaulted, startNow)
    {}

    ~grpc_srv_t()
    {
        stop_and_join();
    }

    auto& services() noexcept { return _services; }

    template<size_t I = 0>
    auto& service() noexcept { return get_elm<I>(_services); }

    template<class S, size_t I = 0>
    auto& service() noexcept { return get_elm<I>(keep_elms(_services, DSK_TYPE_EXP(_derived_from_<T, S>))); }

    template<class S>
    static constexpr bool has_service() noexcept { return ct_contains(to_type_list<Services>(), type_c<S>); }

    auto& cq() noexcept { return _cqs.get(); }
    auto& cq(uint32_t i) noexcept { return _cqs.get(i); }

    bool started() const noexcept { return _cqs.started(); }

    // NOTE: user should NOT call builder.AddCompletionQueue()/BuildAndStart().
    void setup(grpc::ServerBuilder& builder,
               uint32_t nCq = 1,
               threads_per_cq threadsPerCq = threads_per_cq::defaulted)
    {
        DSK_ASSERT(! _srv);
        DSK_ASSERT(_cqs.empty());
        DSK_ASSERT(nCq);

        foreach_elms(_services, [&]<class S>(S& s)
        {
            if constexpr(_derived_from_<S, grpc::AsyncGenericService>)
            {
                builder.RegisterAsyncGenericService(&s);
            }
            else
            {
                builder.RegisterService(&s);
            }
        });

        _cqs.reserve(nCq);

        for(uint32_t i = 0; i< nCq; ++i)
        {
            _cqs.add_cq(builder.AddCompletionQueue(), threadsPerCq);
        }

        _srv = builder.BuildAndStart();
    }

    // start([](grpc::ServerBuilder& builder)
    // {
    //     builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    //     // other settings
    // });
    void setup(std::invocable<grpc::ServerBuilder&> auto&& build,
               uint32_t nCq = 1,
               threads_per_cq threadsPerCq = threads_per_cq::defaulted)
    {
        grpc::ServerBuilder builder;

        build(builder);

        setup(builder, nCq, threadsPerCq);
    }

    void start()
    {
        DSK_ASSERT(_srv);
        _cqs.start();
    }

    void stop()
    {
        if(! started())
            return;

        _srv->Shutdown();
        _cqs.stop();
    }

    // stop() must be called at some point to make join() return.
    void join()
    {
        if(! started())
            return;

        _srv->Wait();

        _cqs.join();

        _srv.reset();
    }

    void stop_and_join()
    {
        stop();
        join();
    }
    
    // Create a task<> to handle grpc_rpc<Method>.
    // The task continuously accepts incoming client RPCs, and spawn tasks to handle them.
    // It should scale well for most use cases.
    // Otherwise, you can try creating more tasks, so there are more acceptors.
    // 
    //      _async_op_ f(allocated_unique_ptr<grpc_rpc<Method, Cq>, allocator_type> [ , request_type])
    //      
    // defined in rpc.hpp
    template<auto Method>
    task<> create_task(auto f);
};


template<class... Services>
using grpc_srv = grpc_srv_t<tuple<Services...>>;

using grpc_generic_srv = grpc_srv<grpc::AsyncGenericService>;


} // namespace dsk
