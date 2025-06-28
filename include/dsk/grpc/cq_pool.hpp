#pragma once

#include <dsk/grpc/cq.hpp>
#include <dsk/util/atomic.hpp>


namespace dsk{


template<
    class Cq = grpc_cq
>
class grpc_cq_pool_t
{
public:
    using allocator_type = typename Cq::allocator_type;

private:
    vector<Cq, allocator_type> _cqs;
    atomic<uint32_t>           _next{0};

public:
    grpc_cq_pool_t() = default;

    explicit grpc_cq_pool_t(uint32_t n,
                            threads_per_cq threadsPerCq = threads_per_cq::defaulted,
                            start_scheduler_e startNow = dont_start_now)
    {
        set_size(n, threadsPerCq);

        if(startNow)
        {
            start();
        }
    }

    bool empty() const noexcept { return _cqs.empty(); }
    uint32_t size() const noexcept { return static_cast<uint32_t>(_cqs.size()); }

    auto& get() noexcept
    {
        DSK_ASSERT(_cqs.size());
        uint32_t const i = _next.fetch_add(1, memory_order_relaxed) % _cqs.size();
        return _cqs[i];
    }

    auto& get(uint32_t i) noexcept
    {
        DSK_ASSERT(i < _cqs.size());
        return _cqs[i];
    }

    // NOTE: shouldn't use with reserve()/add_cq()
    void set_size(uint32_t n, threads_per_cq threadsPerCq = threads_per_cq::defaulted)
    {
        DSK_ASSERT(_cqs.empty());

        _cqs.reserve(n);

        for(uint32_t i = 0; i < n; ++i)
        {
            _cqs.emplace_back(threadsPerCq);
        }
    }

    void reserve(uint32_t n)
    {
        DSK_ASSERT(_cqs.empty());
        _cqs.reserve(n);
    }

    template<_derived_from_<grpc::CompletionQueue> T>
    auto& add_cq(std::unique_ptr<T>&& cq, threads_per_cq threadsPerCq = threads_per_cq::defaulted)
    {
        DSK_ASSERT(! started());
        return _cqs.emplace_back(mut_move(cq), threadsPerCq);
    }

    auto& add_cq(threads_per_cq threadsPerCq = threads_per_cq::defaulted)
    {
        DSK_ASSERT(! started());
        return _cqs.emplace_back(threadsPerCq);
    }

    bool started() const noexcept
    {
        return _cqs.size() && _cqs[0].started();
    }

    void start()
    {
        DSK_ASSERT(_cqs.size());

        for(auto& q : _cqs)
        {
            q.start();
        }
    }

    void stop()
    {
        for(auto& q : _cqs)
        {
            q.stop();
        }
    }

    void join()
    {
        _cqs.clear();
    }

    void stop_and_join()
    {
        // don't use _cqs.clear() only. All cqs' stop() must be called first.
        stop();
        join();
    }
};


using grpc_cq_pool = grpc_cq_pool_t<>;


} // namespace dsk
