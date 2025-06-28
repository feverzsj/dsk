#pragma once

#include <dsk/config.hpp>
#include <dsk/expected.hpp>
#include <dsk/continuation.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/function.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/thread.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/util/condition_variable.hpp>


namespace dsk{


// A somewhat not so naive thread pool,
// based on a simple work stealing implementation.
template<
    class Job = continuation
    //class Job = unique_function<void()>
>
class simple_thread_pool_t
{
    enum err_e { e_ok = 0, e_stop, e_nolock, e_nojob, r_exit };

    class thread_state
    {
        mutex              _mtx;
        condition_variable _cv;
        bool               _stop = false;
        deque<Job>         _jobs;

    public:
        // vector::resize(n) requires move or copy ctor,
        // but they should never be called in our case.
        thread_state() = default;
        thread_state(thread_state&&) noexcept { DSK_ASSERT(true); }
        ///

        std::optional<Job> try_pop()
        {
            unique_lock lk(_mtx, try_to_lock);

            if(! lk || _jobs.empty())
                return {};

            auto job = mut_move(_jobs.front());
            _jobs.pop_front();
            return job;
        }

        std::optional<Job> pop()
        {
            unique_lock lk(_mtx);

            for(;;)
            {
                if(_stop)
                    return {};

                if(_jobs.size())
                    break;

                _cv.wait(lk);
            }

            auto job = mut_move(_jobs.front());
            _jobs.pop_front();
            return job;
        }

        bool try_push(auto&& job)
        {
            unique_lock lk(_mtx, try_to_lock);
            
            if(! lk)
                return false;

            bool wasEmpty = _jobs.empty();

            _jobs.emplace_back(DSK_FORWARD(job));

            if(wasEmpty)
                _cv.notify_one();

            return true;
        }

        void push(auto&& job)
        {
            lock_guard lk(_mtx);

            bool wasEmpty = _jobs.empty();

            _jobs.emplace_back(DSK_FORWARD(job));

            if(wasEmpty)
                _cv.notify_one();
        }

        void stop()
        {
            lock_guard lk(_mtx);
           _stop = true;
           _cv.notify_one();
        }
    };

    vector<thread>       _threads;
    vector<thread_state> _states;
    atomic<uint32_t>     _next = 0;
    uint32_t             _size = 0;
    int                  _maxConcurrency = default_max_concurrency();

public:
    simple_thread_pool_t(simple_thread_pool_t const&) = delete;
    simple_thread_pool_t& operator=(simple_thread_pool_t const&) = delete;

    explicit simple_thread_pool_t(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit simple_thread_pool_t(start_scheduler_e startNow)
        : simple_thread_pool_t(-1, startNow)
    {}

    ~simple_thread_pool_t() { stop_and_join(); }

    template<class T = int>
    static T default_max_concurrency() noexcept
    {
        auto n = thread::hardware_concurrency();

        if(n < 1)
            n = 1;

        return static_cast<T>(n);
    }

    using is_scheduler = void;

    void set_max_concurrency(int n) noexcept
    {
        _maxConcurrency = n > 0 ? n : default_max_concurrency();
    }

    bool started() const noexcept {  return _size > 0; }

    void start()
    {
        DSK_ASSERT(_threads.empty());
        DSK_ASSERT(_states.empty());
        DSK_ASSERT(! _next.load(memory_order_relaxed));
        DSK_ASSERT(! _size);
        DSK_ASSERT(_maxConcurrency > 0);

        _size = static_cast<uint32_t>(_maxConcurrency);

        _states.resize(_size);

        _threads.reserve(_size);

        for(uint32_t index = 0; index < _size; ++index)
        {
            _threads.emplace_back([this, index]()
            {
                for(;;)
                {
                    std::optional<Job> job;

                    // First try to pop from one of threads without blocking.
                    for(uint32_t i = 0; i < _size; ++i)
                    {
                        if((job = _states[(index + i) % _size].try_pop()))
                            break;
                    }

                    // Otherwise, do a blocking pop on own thread.
                    if(! job)
                    {
                        if(! (job = _states[index].pop()))
                            return; // stop requested, only exit own thread
                    }

                    (*job)();
                }
            });
        }
    }

    void restart()
    {
        stop_and_join();
        start();
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        uint32_t index = _next.fetch_add(1, memory_order_relaxed) % _size;

        // First try to push to one of the threads without blocking.
        for(uint32_t i = 0; i < _size; ++i)
        {
            if(_states[(index + i) % _size].try_push(DSK_FORWARD(f)))
                return;
        }

        // Otherwise, do a blocking push on the selected thread.
        _states[index].push(DSK_FORWARD(f));
    }

    // Signal underlying threads to stop. Pending jobs may be ignored.
    void stop()
    {
        if(! started())
            return;

        for(auto& s : _states)
        {
            s.stop();
        }
    }

    // Join underlying threads.
    // stop() must be called at some point to make join() return.
    // The pool can be started again after join().
    void join()
    {
        if(! started())
            return;

        for(auto& t : _threads)
        {
            t.join();
        }

        _threads.clear();
        _states.clear();
        _next.store(0, memory_order_relaxed);
        _size = 0;
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


using simple_thread_pool = simple_thread_pool_t<>;


// a single lock round-robin style thread pool
template<
    class Job = continuation
    //class Job = unique_function<void()>
>
class naive_thread_pool_t
{
    int                _maxConcurrency = default_max_concurrency();
    bool               _stop = false;
    mutex              _mtx;
    condition_variable _cv;
    vector<thread>     _threads;
    deque<Job>         _jobs;

public:
    naive_thread_pool_t(naive_thread_pool_t const&) = delete;
    naive_thread_pool_t& operator=(naive_thread_pool_t const&) = delete;

    explicit naive_thread_pool_t(int n = -1, start_scheduler_e startNow = dont_start_now)
    {
        set_max_concurrency(n);

        if(startNow)
        {
            start();
        }
    }

    explicit naive_thread_pool_t(start_scheduler_e startNow)
        : naive_thread_pool_t(-1, startNow)
    {}

    ~naive_thread_pool_t() { stop_and_join(); }

    template<class T = int>
    static T default_max_concurrency() noexcept
    {
        auto n = thread::hardware_concurrency();

        if(n < 1)
            n = 1;

        return static_cast<T>(n);
    }

    using is_scheduler = void;

    void set_max_concurrency(int n) noexcept
    {
        _maxConcurrency = n > 0 ? n : default_max_concurrency();
    }

    bool started() const noexcept {  return _threads.size() > 0; }

    void start()
    {
        DSK_ASSERT(_threads.empty());
        DSK_ASSERT(_jobs.empty());
        DSK_ASSERT(! _stop);
        DSK_ASSERT(_maxConcurrency > 0);

        _threads.reserve(_maxConcurrency);

        for(int i = 0; i < _maxConcurrency; ++i)
        {
            _threads.emplace_back([this]()
            {
                for(;;)
                {
                    std::optional<Job> job;

                    {
                        unique_lock lock(_mtx);

                        for(;;)
                        {
                            if(_stop)
                                return;

                            if(_jobs.size())
                                break;
                            
                            _cv.wait(lock);
                        }

                        job = mut_move(_jobs.front());
                        _jobs.pop_front();
                    }

                    (*job)();
                }
            });
        }
    }

    void restart()
    {
        stop_and_join();
        start();
    }

    void post(auto&& f)
    {
        DSK_ASSERT(started());

        {
            unique_lock lock(_mtx);
            _jobs.emplace_back(DSK_FORWARD(f));
        };

        _cv.notify_one();
    }

    // Signal underlying threads to stop. Pending jobs may be ignored.
    void stop()
    {
        if(! started())
            return;

        {
            unique_lock lock(_mtx);

            if(_stop)
                return;

            _stop = true;
        };

        _cv.notify_all();
    }

    // Join underlying threads.
    // stop() must be called at some point to make join() return.
    // The pool can be started again after join().
    void join()
    {
        if(! started())
            return;

        for(auto& t : _threads)
        {
            t.join();
        }

        _threads.clear();
        _jobs.clear();
        _stop = false;
    }

    void stop_and_join()
    {
        stop();
        join();
    }
};


using naive_thread_pool = naive_thread_pool_t<>;


} // namespace dsk
