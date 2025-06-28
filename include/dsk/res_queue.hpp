#pragma once

#include <dsk/async_op.hpp>
#include <dsk/any_resumer.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/range.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/unordered.hpp>
#include <deque>
#include <vector>
#include <utility>
#include <optional>


namespace dsk{


struct res_queue_stats
{
    size_t enqueueTotal = 0; //  total enqueue requests
    size_t enqueueWait  = 0; // failed enqueue requests
    size_t dequeueTotal = 0; //  total dequeue requests
    size_t dequeueWait  = 0; // failed dequeue requests

    constexpr void reset() noexcept { *this = {}; }

    constexpr double enqueue_no_wait() const noexcept { return enqueueTotal - enqueueWait; }
    constexpr double dequeue_no_wait() const noexcept { return dequeueTotal - dequeueWait; }

    constexpr double enqueue_wait_rate() const noexcept { return static_cast<double>(enqueueWait) / enqueueTotal; }
    constexpr double dequeue_wait_rate() const noexcept { return static_cast<double>(dequeueWait) / dequeueTotal; }

    constexpr double enqueue_no_wait_rate() const noexcept { return static_cast<double>(enqueue_no_wait()) / enqueueTotal; }
    constexpr double dequeue_no_wait_rate() const noexcept { return static_cast<double>(dequeue_no_wait()) / dequeueTotal; }
};


template<class T>
class res_queue
{
    template<bool IsEnqueue>
    class async_queue_op
    {
    public:
        res_queue*                        _rq = nullptr;
        DSK_DEF_MEMBER_IF(IsEnqueue, T)   _v;
        std::optional<
            expected<
                std::conditional_t<
                    IsEnqueue, void, T>,
                errc>>                    _r;
        any_resumer                       _resumer;
        continuation                      _cont;
        optional_stop_callback            _scb; // must be last one defined

        explicit async_queue_op(res_queue* p, auto&&... args)
            : _rq(p), _v(DSK_FORWARD(args)...)
        {
            static_assert(IsEnqueue || ! sizeof...(args));
        }

        using is_async_op = void;

        bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
        {
            DSK_ASSERT(! _r);

            if(stop_requested(ctx))
            {
                _r.emplace(unexpect, errc::canceled);
                return false;
            }

            {
                unique_lock lk(_rq->_mtx);

                if constexpr(IsEnqueue)
                {
                    if(auto r = _rq->try_enqueue_no_lock(mut_move(_v)))
                    {
                        lk.unlock();

                        if(*r)
                        {
                            (*r)->complete(mut_move(_v));
                        }

                        _r.emplace(expect);
                        return false;
                    }
                    else if(r.error() != errc::out_of_capacity)
                    {
                        _r.emplace(unexpect, r.error());
                        return false;
                    }
                }
                else // dequeue
                {
                    if(auto r = _rq->try_dequeue_no_lock())
                    {
                        lk.unlock();

                        if(r->first)
                        {
                            r->first->complete();
                        }

                        _r.emplace(expect, mut_move(r->second));
                        return false;
                    }
                    else if(r.error() != errc::resource_unavailable)
                    {
                        _r.emplace(unexpect, r.error());
                        return false;
                    }
                }

                _resumer = get_resumer(ctx);
                _cont = DSK_FORWARD(cont);

                if(stop_possible(ctx))
                {
                    _scb.emplace(get_stop_token(ctx), [this]()
                    {
                        bool thisExists = [&]()
                        {
                            lock_guard lg(_rq->_mtx);
                            return _rq->get_waiters<IsEnqueue>().erase(this);
                        }();

                        if(thisExists)
                        {
                            DSK_ASSERT(! _r);

                            //_scb.reset();
                            _r.emplace(unexpect, errc::canceled);
                            resume(mut_move(_cont), _resumer);
                        }
                    });
                }

                _rq->get_waiters<IsEnqueue>().add(this); // when initiate() gets called, this op should be
            }                                            // in its final place, so its address shouldn't change.

            return true;
        }

        bool is_failed() const noexcept
        {
            DSK_ASSERT(_r);
            return has_err(*_r);
        }

        auto take_result() noexcept
        {
            DSK_ASSERT(_r);
            return *mut_move(_r);
        }

        // when invoked, this op should have been removed from queue, but still on same address,
        // so request_stop() won't affect result at this point.
        void complete(auto&&... args)
        {
            static_assert(! IsEnqueue || ! sizeof...(args));
            DSK_ASSERT(! _r);

            //_scb.reset();
            _r.emplace(expect, DSK_FORWARD(args)...);
            resume(mut_move(_cont), _resumer);
        }

        void complete_with_err(errc e)
        {
            DSK_ASSERT(! _r);

            //_scb.reset();
            _r.emplace(unexpect, e);
            resume(mut_move(_cont), _resumer);
        }
    };

    using async_enqueue_op = async_queue_op<true>;
    using async_dequeue_op = async_queue_op<false>;

    template<bool IsEnqueue>
    struct waiter_queue
    {
        using op_type = async_queue_op<IsEnqueue>;

        deque<op_type*> waiterQueue;
        //unstable_unordered_map<op_type*, op_type**> waiters; // map to waiterQueue item.

        size_t size() const noexcept { return waiterQueue.size(); }
        bool  empty() const noexcept { return waiterQueue.empty(); }

        void add(op_type* w)
        {
            DSK_ASSERT(w);
            /*auto& b =*/ waiterQueue.emplace_back(w);
            //DSK_VERIFY(waiters.emplace(w, &b).second);
        }

        op_type* take_oldest()
        {
            while(waiterQueue.size() && ! waiterQueue.front())
            {
                waiterQueue.pop_front(); // pop item marked as deleted
            }

            if(waiterQueue.empty())
            {
                return nullptr;
            }

            auto* w = waiterQueue.front();
            //DSK_VERIFY(waiters.erase(w));
            waiterQueue.pop_front();

            return w;
        }

        // NOTE: may contain null ops.
        deque<op_type*> take_all()
        {
            //waiters.clear();
            return mut_move(waiterQueue);
        }

        bool erase(op_type* w)
        {
            //if(auto it = waiters.find(w); it != waiters.end())
            //{
            //    *(it->second) = nullptr; // mark waiterQueue item as deleted
            //    waiters.erase(it);
            //    return true;
            //}

            // linear search. waiterQueue should be relatively small
            if(auto it = std::ranges::find(waiterQueue, w); it != waiterQueue.end())
            {
                *it = nullptr; // mark as deleted
                return true;
            }

            return false;
        }
    };

    mutex    _mtx;
    size_t   _cap = 1;
    bool     _endMarked = false;
    deque<T> _resQueue;
    waiter_queue<true > _enqueueWaiters;
    waiter_queue<false> _dequeueWaiters;
    res_queue_stats     _stats;

    template<bool IsEnqueue>
    auto& get_waiters() noexcept
    {
        if constexpr(IsEnqueue) return _enqueueWaiters;
        else                    return _dequeueWaiters;
    }

    void integrity_assert_no_lock() const
    {
    #ifndef NDEBUG
        if(_resQueue.empty()) DSK_ASSERT(_enqueueWaiters.empty());
        else                  DSK_ASSERT(_dequeueWaiters.empty());
    #endif
    }

    expected<async_dequeue_op*, errc> try_enqueue_no_lock(auto&&... args)
    {
        integrity_assert_no_lock();

        if(_endMarked)
        {
            return errc::end_reached;
        }

        ++_stats.enqueueTotal;

        if(_resQueue.size() >= _cap)
        {
            ++_stats.enqueueWait;
            return errc::out_of_capacity;
        }

        if(auto* w = _dequeueWaiters.take_oldest())
        {
            return w;
        }

        _resQueue.emplace_back(DSK_FORWARD(args)...);
        return nullptr;
    }

    expected<std::pair<async_enqueue_op*, T>, errc> try_dequeue_no_lock()
    {
        integrity_assert_no_lock();

        ++_stats.dequeueTotal;

        if(_resQueue.empty())
        {
            ++_stats.dequeueWait;

            if(_endMarked) return errc::end_reached;
            else           return errc::resource_unavailable;
        }

        T v = mut_move(_resQueue.front());
        _resQueue.pop_front();

        auto* w = _enqueueWaiters.take_oldest();

        if(w)
        {
            _resQueue.emplace_back(mut_move(w->_v));
        }

        return {expect, w, mut_move(v)};
    }

public:
    explicit res_queue(size_t cap)
        : _cap(cap)
    {
        DSK_ASSERT(_cap > 0);
    }

    res_queue_stats const& stats() const noexcept
    {
        return _stats;
    }

    size_t capacity() const
    {
        lock_guard lg(_mtx);
        return _cap;
    }

    void increase_capacity(size_t n)
    {
        lock_guard lg(_mtx);
        DSK_ASSERT(SIZE_MAX - _cap >= n);
        _cap += n;            
    }

    void clear()
    {
        lock_guard lg(_mtx);
        _resQueue.clear();
    }

    // Mark end of queue.
    // Current dequeue waiters will be resumed with errc::end_reached.
    // Dequeue ops will return errc::end_reached until all items have been retrived.
    // Enqueue ops will fail with errc::end_reached.
    bool mark_end()
    {
        bool isFirst = false;

        auto waiters = [&]()
        {
            lock_guard lg(_mtx);
            integrity_assert_no_lock();

            if(! _endMarked)
            {
                _endMarked = true;
                isFirst = true;

                return _dequeueWaiters.take_all();
            }

            return decltype(_dequeueWaiters.take_all())();
        }();

        for(auto* w : waiters)
        {
            if(w)
            {
                w->complete_with_err(errc::end_reached);
            }
        }

        return isFirst;
    }

    void clear_end_mark()
    {
        lock_guard lg(_mtx);
        _endMarked = false;
    }

    auto enqueue(auto&&... args)
    {
        return async_enqueue_op(this, DSK_FORWARD(args)...);
    }

    auto dequeue()
    {
        return async_dequeue_op(this);
    }

    errc try_enqueue(auto&&... args)
    {
        unique_lock lk(_mtx);

        DSK_E_TRY(async_dequeue_op* dequeueOp, try_enqueue_no_lock(DSK_FORWARD(args)...));

        lk.unlock();

        if(dequeueOp)
        {
            dequeueOp->complete(DSK_FORWARD(args)...);
        }

        return {};
    }

    expected<T, errc> try_dequeue()
    {
        unique_lock lk(_mtx);

        DSK_E_TRY_FWD(DSK_PARAM([enqueueOp, v]), try_dequeue_no_lock());

        lk.unlock();

        if(enqueueOp)
        {
            enqueueOp->complete();
        }

        return mut_move(v);
    }

    // NOTE: elements in 'r' are std::moved in to the queue.
    errc force_enqueue_range(std::ranges::range auto&& r)
    {
        if(std::ranges::empty(r))
        {
            return {};
        }

        vector<std::pair<async_dequeue_op*, T>> waiters;

        {
            unique_lock lk(_mtx);
            integrity_assert_no_lock();

            if(_endMarked)
            {
                return errc::end_reached;
            }

            ++_stats.enqueueTotal;

            _resQueue.append_range(move_range(r));

            waiters.reserve(std::min(_dequeueWaiters.size(), _resQueue.size()));

            while(auto* w = _dequeueWaiters.take_oldest())
            {
                waiters.emplace_back(w, mut_move(_resQueue.front()));
                _resQueue.pop_front();

                if(_resQueue.empty())
                {
                    break;
                }
            }
        }

        for(auto&[w, v] : waiters)
        {
            w->complete(mut_move(v));
        }

        return {};
    }

    // One can use it after DSK_TRY dequeue() to make the whole awaitable.
    errc force_dequeue_all(auto& cont)
    {
        vector<async_enqueue_op*> waiters;

        {
            unique_lock lk(_mtx);
            integrity_assert_no_lock();

            if(_resQueue.empty())
            {
                if(_endMarked) return errc::end_reached;
                else           return {};
            }

            ++_stats.dequeueTotal;

            cont.append_range(move_range(_resQueue));
            _resQueue.clear();

            waiters.reserve(std::min(_enqueueWaiters.size(), _cap));

            while(auto* w = _enqueueWaiters.take_oldest())
            {
                _resQueue.emplace_back(mut_move(w->_v));

                waiters.emplace_back(w);

                if(_resQueue.size() >= _cap)
                {
                    break;
                }
            }
        }

        for(auto* w : waiters)
        {
            w->complete();
        }

        return {};
    }

    template<class Cont = vector<T>>
    expected<Cont, errc> force_dequeue_all()
    {
        Cont cont;
        DSK_E_TRY_ONLY(force_dequeue_all(cont));
        return cont;
    }
};


} // namespace dsk
