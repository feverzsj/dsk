#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/tbb/allocator.hpp>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <tbb/global_control.h>


namespace dsk{


// NOTE: thread workers are shared globally or per processor.
//       set_max_concurrency() is also constrianed by global setting.
class tbb_thread_pool
{
    tbb::task_arena              _arena; // for limiting resources
    tbb::task_group_context      _ctx{tbb::task_group_context::isolated}; // for canceling jobs
    tbb::task_group              _tg{_ctx}; // dispatching and tracking jobs
    tbb::task_arena::constraints _opts;

public:
    tbb_thread_pool(tbb_thread_pool const&) = delete;
    tbb_thread_pool& operator=(tbb_thread_pool const&) = delete;

    explicit tbb_thread_pool(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit tbb_thread_pool(start_scheduler_e startNow)
        : tbb_thread_pool(-1, startNow)
    {}

    ~tbb_thread_pool() { wait(); }

    tbb_thread_pool& set_constraints(tbb::task_arena::constraints const& opts)
    {
        _opts = opts;
        return *this;
    }

    tbb_thread_pool& set_numa_id(tbb::numa_node_id id)
    {
        _opts.set_numa_id(id);
        return *this;
    }

    tbb_thread_pool& set_core_type(tbb::core_type_id id)
    {
        _opts.set_core_type(id);
        return *this;
    }

    tbb_thread_pool& set_max_threads_per_core(int n)
    {
        _opts.set_max_threads_per_core(n);
        return *this;
    }

    using is_scheduler = void;
    using allocator_type = tbb_scalable_allocator<void>;

    tbb_thread_pool& set_max_concurrency(int n) noexcept
    {
        _opts.set_max_concurrency(n);
        return *this;
    }

    bool started() const noexcept {  return _arena.is_active(); }

    void start(unsigned reservedForMasters = 1,
               tbb::task_arena::priority priority = tbb::task_arena::priority::normal)
    {
        DSK_ASSERT(! _arena.is_active());
        DSK_ASSERT(_opts.max_concurrency);

        _arena.initialize(_opts, reservedForMasters, priority);
    }

    void restart(unsigned reservedForMasters = 1,
                 tbb::task_arena::priority priority = tbb::task_arena::priority::normal)
    {
        join();
        start(reservedForMasters, priority);
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        _arena.execute([&]()
        {
            // tbb only uses functor's const operator()
            if constexpr(requires{ std::as_const(f)(); })
            {
                _tg.run(DSK_FORWARD(f));
            }
            else
            {
                _tg.run([f = DSK_FORWARD(f)]()
                {
                    const_cast<DSK_NO_CV_T(f)&>(f)();
                });
            }
        });

        //_arena.execute([&]() mutable { _tg.run(_tg.defer(DSK_FORWARD(f))); });

        // enqueue uses different mechinism than task_group.run(), and it's much slower.
        //_arena.enqueue(_tg.defer(DSK_FORWARD(f)));
    }

    // signal stop executing jobs, pending jobs may be ignored.
    void stop()
    {
        if(started())
        {
            _arena.execute([this](){ _ctx.cancel_group_execution(); });
        }
    }

    // Wait for all outstanding jobs to complete, and remove reference from internal arena representation.
    // The pool can be started again after join();
    void join()
    {
        wait();
        _arena.terminate();
    }

    void stop_and_join()
    {
        stop();
        join();
    }

    // Wait for all outstanding jobs to complete
    void wait()
    {
        if(started())
        {
            _arena.execute([this](){ _tg.wait(); }); // _tg.wait() also resets context cancellation state.
        }
    }
};


// uses implicit task_arena, which may change across execution.
// NOTE: set_max_concurrency() will change global setting when start.
class tbb_implicit_thread_pool
{
    tbb::task_group_context _ctx{tbb::task_group_context::isolated}; // for canceling jobs
    tbb::task_group         _tg{_ctx}; // dispatching and tracking jobs
    bool                    _started = false;
    int                                _maxConcurrency = -1;
    std::optional<tbb::global_control> _maxConcurrencyCtrl;

public:
    tbb_implicit_thread_pool(tbb_implicit_thread_pool const&) = delete;
    tbb_implicit_thread_pool& operator=(tbb_implicit_thread_pool const&) = delete;

    explicit tbb_implicit_thread_pool(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit tbb_implicit_thread_pool(start_scheduler_e startNow)
        : tbb_implicit_thread_pool(-1, startNow)
    {}

    ~tbb_implicit_thread_pool() { wait(); }

    using is_scheduler = void;
    using allocator_type = tbb_scalable_allocator<void>;

    void set_max_concurrency(int n) noexcept
    {
        _maxConcurrency = n;
    }

    bool started() const noexcept {  return _started; }

    void start()
    {
        DSK_ASSERT(! _started);
        DSK_ASSERT(! _maxConcurrencyCtrl);
        DSK_ASSERT(! _ctx.is_group_execution_cancelled());

        if(_maxConcurrency > 0)
        {
            _maxConcurrencyCtrl.emplace(tbb::global_control::max_allowed_parallelism,
                                        static_cast<size_t>(_maxConcurrency));
        }

        _started = true;
    }

    void restart()
    {
        join();
        start();
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        // tbb only uses functor's const operator()
        if constexpr(requires{ std::as_const(f)(); })
        {
            _tg.run(DSK_FORWARD(f));
        }
        else
        {
            _tg.run([f = DSK_FORWARD(f)]()
            {
                const_cast<DSK_NO_CV_T(f)&>(f)();
            });
        }
    }

    // signal stop executing jobs, pending jobs may be ignored.
    void stop()
    {
        if(started())
        {
            _ctx.cancel_group_execution();
        }
    }

    // Wait for all outstanding jobs to complete, and remove global_control setting.
    // The pool can be started again after join();
    void join()
    {
        if(started())
        {
            _tg.wait(); // also resets context cancellation state.
            _maxConcurrencyCtrl.reset();
            _started = false;
        }
    }

    void stop_and_join()
    {
        stop();
        join();
    }

    // Wait for all outstanding jobs to complete
    void wait()
    {
        if(started())
        {
            _tg.wait(); // also resets context cancellation state.
        }
    }
};


} // namespace dsk
