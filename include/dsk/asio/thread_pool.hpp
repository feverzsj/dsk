#pragma once

#include <dsk/util/debug.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/thread.hpp>
#include <dsk/asio/config.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/recycling_allocator.hpp>
#include <boost/asio/bind_allocator.hpp>
#include <optional>


namespace dsk{


class asio_thread_pool
{
    std::optional<asio::thread_pool> _pool;
    int _maxConcurrency = -1;

public:
    asio_thread_pool(asio_thread_pool const&) = delete;
    asio_thread_pool& operator=(asio_thread_pool const&) = delete;

    explicit asio_thread_pool(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit asio_thread_pool(start_scheduler_e startNow)
        : asio_thread_pool(-1, startNow)
    {}

    ~asio_thread_pool() { join(); }

    using is_scheduler = void;
    using allocator_type = DSK_DEFAULT_ALLOCATOR<void>; //asio::recycling_allocator<void>;

    void set_max_concurrency(int n) noexcept
    {
        _maxConcurrency = n;
    }

    bool started() const noexcept {  return _pool.has_value(); }

    void start()
    {
        DSK_ASSERT(! _pool);

        if(_maxConcurrency > 0) _pool.emplace(static_cast<size_t>(_maxConcurrency));
        else                    _pool.emplace();
    }

    void restart()
    {
        join();
        start();
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        asio::post(*_pool, DSK_FORWARD(f));
        //asio::post(*_pool, asio::bind_allocator(allocator_type(), DSK_FORWARD(f)));
    }

    // Signal underlying threads to stop. Pending jobs may be ignored.
    void stop()
    {
        if(_pool)
        {
            _pool->stop();
        }
    }

    // Wait for all outstanding jobs to complete, and exit threads.
    // The pool can be started again after join();
    void join()
    {
        if(_pool)
        {
            _pool->join();
            _pool.reset();
        }
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


template<class T>
concept _io_scheduler_ = requires{ typename std::remove_cvref_t<T>::is_io_scheduler; } && _scheduler_<T>;


// io_context run on threads
class asio_io_thread_pool 
{
    asio::io_context _ctx; // must be first, as resume(..., curSrId) uses executor's context as curSrId.
    deque<thread>    _threads;
    int              _maxConcurrency = default_max_concurrency();

public:
    asio_io_thread_pool(asio_io_thread_pool const&) = delete;
    asio_io_thread_pool& operator=(asio_io_thread_pool const&) = delete;

    explicit asio_io_thread_pool(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit asio_io_thread_pool(start_scheduler_e startNow)
        : asio_io_thread_pool(-1, startNow)
    {}

    ~asio_io_thread_pool() { join(); }

    template<class T = int>
    static T default_max_concurrency() noexcept
    {
        auto n = thread::hardware_concurrency() * 2;

        if(n < 1)
            n = 2;

        return static_cast<T>(n);
    }

    using is_io_scheduler = void;
    auto& context()       noexcept { return _ctx; }
    auto& context() const noexcept { return _ctx; }

    using is_scheduler = void;
    using allocator_type = DSK_DEFAULT_ALLOCATOR<void>; //asio::recycling_allocator<void>;

    void set_max_concurrency(int n) noexcept
    {
        _maxConcurrency = n > 0 ? n : default_max_concurrency();
    }

    bool started() const noexcept {  return _threads.size() > 0; }

    void start()
    {
        DSK_ASSERT(_threads.empty());
        DSK_ASSERT(! _ctx.stopped());
        DSK_ASSERT(_maxConcurrency > 0);

        _ctx.get_executor().on_work_started(); // avoid run() from returning when there is no job

        for(int i = 0; i < _maxConcurrency; ++i)
        {
            _threads.emplace_back([this]()
            {
                try
                {
                    _ctx.run();
                }
                catch(...)
                {
                    DSK_ABORT("unhandled expcetion");
                }
            });
        }
    }

    void restart()
    {
        join();
        start();
    }

    void post(auto&& f)
    {
        // It's ok to post on a not started io_context,
        // but we want it to be consist with other schedulers,
        // i.e., you should only post to started scheduler.
        DSK_ASSERT(started());

        asio::post(_ctx, DSK_FORWARD(f));
        //asio::post(_ctx, asio::bind_allocator(allocator_type(), DSK_FORWARD(f)));
    }

    // Signal underlying threads to stop. Pending jobs may be ignored.
    void stop()
    {
        if(started())
        {
            _ctx.stop();
        }
    }

    // Wait for all outstanding jobs to complete, and exit threads.
    // The pool can be started again after join();
    void join()
    {
        if(! started())
            return;

        _ctx.get_executor().on_work_finished(); // allow run() to return when there is no job

        for(auto& t : _threads)
        {
            t.join();
        }

        _threads.clear();

        DSK_ASSERT(_ctx.stopped());

        _ctx.restart();
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


} // namespace dsk
