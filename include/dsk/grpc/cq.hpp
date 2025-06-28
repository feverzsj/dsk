#pragma once

#include <dsk/util/vector.hpp>
#include <dsk/util/thread.hpp>
#include <dsk/util/function.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <dsk/default_allocator.hpp>

#include <grpcpp/completion_queue.h>


namespace dsk{


enum class threads_per_cq : uint32_t
{
    defaulted = 2
};


template<
    class Tag = unique_function<void(bool)>,
    class Alloc = DSK_DEFAULT_ALLOCATOR<void>
>
class grpc_cq_t
{
    static_assert(is_stateless_allocator_v<Alloc>);

    uint32_t                               _nThread = std::to_underlying(threads_per_cq::defaulted);
    vector<thread, Alloc>                  _threads;
    std::unique_ptr<grpc::CompletionQueue> _cq;

#ifndef NDEBUG
    bool _isSrv = false;
#endif

public:
    using allocator_type = Alloc;

    explicit grpc_cq_t(std::nullptr_t){}

    template<_derived_from_<grpc::CompletionQueue> T>
    explicit grpc_cq_t(std::unique_ptr<T>&& cq,
                       threads_per_cq nThreads = threads_per_cq::defaulted,
                       start_scheduler_e startNow = dont_start_now)
    {
        set_cq(mut_move(cq));
        set_thread_cnt(nThreads);

        if(startNow)
        {
            start();
        }
    }

    template<_derived_from_<grpc::CompletionQueue> T>
    explicit grpc_cq_t(std::unique_ptr<T>&& cq, start_scheduler_e startNow)
        : grpc_cq_t(mut_move(cq), threads_per_cq::defaulted, startNow)
    {}

    explicit grpc_cq_t(threads_per_cq nThreads = threads_per_cq::defaulted, start_scheduler_e startNow = dont_start_now)
        : grpc_cq_t(std::make_unique<grpc::CompletionQueue>(), nThreads, startNow)
    {}

    explicit grpc_cq_t(start_scheduler_e startNow)
        : grpc_cq_t(threads_per_cq::defaulted, startNow)
    {}

    grpc_cq_t(grpc_cq_t&&) = default; // need this, as dtor disables implicitly generated move ctor.

    ~grpc_cq_t() { stop_and_join(); }

    auto* cq() noexcept { return _cq.get(); }
    auto* srv_cq() noexcept
    {
        DSK_ASSERT(_isSrv);
        return static_cast<grpc::ServerCompletionQueue*>(_cq.get());
    }

    template<_derived_from_<grpc::CompletionQueue> T>
    void set_cq(std::unique_ptr<T>&& cq)
    {
        DSK_ASSERT(! started());
        _cq = mut_move(cq);

    #ifndef NDEBUG
        _isSrv = _derived_from_<T, grpc::ServerCompletionQueue>;
    #endif
    }

    template<class T = int>
    static constexpr T default_max_concurrency() noexcept
    {
        return static_cast<T>(2);
    }

    void set_thread_cnt(uint32_t n) noexcept
    {
        _nThread = n > 0 ? n : std::to_underlying(threads_per_cq::defaulted);
    }

    void set_thread_cnt(threads_per_cq n) noexcept
    {
        set_thread_cnt(std::to_underlying(n));
    }

    bool started() const noexcept {  return _threads.size() > 0; }

    void start()
    {
        DSK_ASSERT(_cq);
        DSK_ASSERT(_threads.empty());
        DSK_ASSERT(_nThread > 0);

        _threads.reserve(_nThread);

        for(uint32_t i = 0; i < _nThread; ++i)
        {
            _threads.emplace_back([this]()
            {
                void* tag;
                bool ok;

                while(_cq->Next(&tag, &ok))
                {
                    try
                    {
                        allocated_unique_ptr<Tag, Alloc> ptag(static_cast<Tag*>((tag)));
                        (*ptag)(ok);
                    }
                    catch(...)
                    {
                        DSK_ABORT("grpc_cq_t.thread_loop: unhandled expcetion");
                    }
                }
            });
        }
    }

    // init will be invoked with tag.
    void post(auto&& init, auto&& onCompletion)
    {
        DSK_ASSERT(started());

        auto ptag = allocate_unique<Tag>(Alloc(), DSK_FORWARD(onCompletion));

        init(ptag.get());

        [[maybe_unused]]auto* p = ptag.release();
    }

    void stop()
    {
        if(! started())
            return;

        _cq->Shutdown();
    }

    // Join underlying threads.
    // stop() must be called at some point to make join() return.
    // The underlying cq will be destroyed on return.
    // To restart, a new cq need be set.
    void join()
    {
        if(! started())
            return;

        for(auto& t : _threads)
        {
            t.join();
        }

        _threads.clear();
        _cq.reset();
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


using grpc_cq = grpc_cq_t<>;


} // namespace dsk
