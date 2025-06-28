#pragma once

#include <dsk/config.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <memory>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace dsk{


template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
class win_thread_pool_t
{
    static_assert(is_stateless_allocator_v<Alloc>);

    PTP_POOL            _pool = nullptr;
    PTP_CLEANUP_GROUP   _cgp = nullptr;
    TP_CALLBACK_ENVIRON _env;
    atomic<bool>        _stop = false;

    int _minConcurrency = -1;
    int _maxConcurrency = -1;

public:
    win_thread_pool_t(win_thread_pool_t const&) = delete;
    win_thread_pool_t& operator=(win_thread_pool_t const&) = delete;

    explicit win_thread_pool_t(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit win_thread_pool_t(start_scheduler_e startNow)
        : win_thread_pool_t(-1, startNow)
    {}

    ~win_thread_pool_t() { stop_and_join(); }

    void set_min_concurrency(int n) noexcept { _minConcurrency = n; }

    using is_scheduler = void;
    using allocator_type = Alloc;

    void set_max_concurrency(int n) noexcept { _maxConcurrency = n; }

    bool started() const noexcept {  return _pool != nullptr; }

    void start()
    {
        DSK_ASSERT(! _pool);
        DSK_ASSERT(! _cgp);
        DSK_ASSERT(! _stop.load(memory_order_relaxed));

        _pool = CreateThreadpool(nullptr);

        if(! _pool)
        {
            throw std::bad_alloc();
        }

        _cgp = CreateThreadpoolCleanupGroup();

        if(! _cgp)
        {
            CloseThreadpool(_pool);
            throw std::bad_alloc();
        }

        InitializeThreadpoolEnvironment(&_env);
        SetThreadpoolCallbackPool(&_env, _pool);
        SetThreadpoolCallbackCleanupGroup(&_env, _cgp, nullptr);

        if(_minConcurrency > 0) DSK_VERIFY(SetThreadpoolThreadMinimum(_pool, static_cast<DWORD>(_minConcurrency)));
        if(_maxConcurrency > 0)            SetThreadpoolThreadMaximum(_pool, static_cast<DWORD>(_maxConcurrency)) ;
    }

    void restart()
    {
        stop_and_join();
        start();
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        if(_stop.load(memory_order_relaxed))
            return;

        auto gen_cb = [&]()
        {
            return [&_stop = _stop, f = DSK_FORWARD(f)]() mutable
            {
                if(! _stop.load(memory_order_relaxed))
                    f();
            };
        };

        using cb_type = decltype(gen_cb());

        auto cb = allocate_unique(Alloc(), gen_cb());

        PTP_WORK work = CreateThreadpoolWork
        (
            [](PTP_CALLBACK_INSTANCE, PVOID d, PTP_WORK work)
            {
                try
                {
                    decltype(cb) cb(static_cast<cb_type*>(d));
                    (*cb)();
                }
                catch(...)
                {
                    DSK_ABORT("win_thread_pool_t.work: unhandled expcetion");
                }

                CloseThreadpoolWork(work);
            },
            cb.get(),
            &_env
        );

        if(! work)
        {
            throw std::bad_alloc();
        }

        [[maybe_unused]]auto* p = cb.release();
        SubmitThreadpoolWork(work);
    }

    // signal threads to stop asap, ignore pending jobs.
    void stop()
    {
        if(! started())
            return;

        if(! _stop.exchange(true, memory_order_relaxed))
        {
            _stop.notify_one();
        }
    }

    // Join underlying threads.
    // stop() must be called at some point to make join() return.
    // The pool can be started again after join().
    void join()
    {
        if(! started())
            return;

        _stop.wait(false, memory_order_relaxed);

        // NOTE: when cleanup starts, new jobs should not be added to the pool.
        // NOTE: pending jobs should not be cancelled by passing TRUE to 2nd arg,
        //       as they need to be deallocated.
        CloseThreadpoolCleanupGroupMembers(_cgp, FALSE, nullptr);
        CloseThreadpoolCleanupGroup(_cgp);
        _cgp = nullptr;

        CloseThreadpool(_pool);
        _pool = nullptr;

        DestroyThreadpoolEnvironment(&_env);

        _stop.store(false, memory_order_relaxed);
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


using win_thread_pool = win_thread_pool_t<>;


} // namespace dsk
