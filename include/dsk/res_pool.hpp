#pragma once

#include <dsk/async_op.hpp>
#include <dsk/any_resumer.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/list.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/conditional_lock.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/unordered.hpp>
#include <utility>
#include <optional>


namespace dsk{


enum res_pool_map_lock_policy
{
    res_pool_map_single_lock,
    res_pool_map_lock_per_pool
};

enum res_pool_map_add_pool_policy
{
    res_pool_map_no_auto_add_pool = false,
    res_pool_map_auto_add_pool = true
};


template<class Cont>
struct res_front_emplacer
{
    Cont& cont;

    auto& operator()(auto&&... args)
    {
        return cont.emplace_front(DSK_FORWARD(args)...);
    }
};

struct default_res_creator
{
    template<class Cont>
    void operator()(res_front_emplacer<Cont> emplacer)
    {
        emplacer();
    }
};


// T can be void
template<
    class T        = void,
    class Creator  = default_res_creator,
    class Recycler = null_op_t,
    res_pool_map_lock_policy LockPolicy = res_pool_map_lock_per_pool
>
class res_pool
{
    static constexpr bool has_res = ! _void_<T>;
    static constexpr bool lock_outside = (LockPolicy != res_pool_map_lock_per_pool);
    static_assert(is_oneof(LockPolicy, res_pool_map_single_lock, res_pool_map_lock_per_pool));

    template<
        class, class, class, class,
        res_pool_map_add_pool_policy,
        res_pool_map_lock_policy
    >
    friend class res_pool_map;

    using list_t   = DSK_DECL_TYPE_IF(has_res, list<T>);
    using res_iter = DSK_CONDITIONAL_T(has_res, typename list_t::iterator, size_t);

    class res_ref
    {
        friend res_pool;

        res_pool& _pool;
        res_iter  _it{};

        res_ref(res_pool& pool, res_iter it) noexcept
            : _pool(pool), _it(it)
        {}

        res_iter release() noexcept
        {
            return std::exchange(_it, _pool.invalid_iter());
        }

    public:
        ~res_ref() { recycle(); }

        res_ref(res_ref&& r) noexcept
            : _pool(r._pool), _it(r.release())
        {}

        res_ref& operator=(res_ref&& r)
        {
            if(this != std::addressof(r))
            {
                DSK_ASSERT(std::addressof(_pool) == std::addressof(r._pool));

                recycle();
                _it = r.release();
            }

            return *this;
        }

        bool valid() const noexcept
        {
            return _it != _pool.invalid_iter();
        }

        explicit operator bool() const noexcept
        {
            return valid();
        }

        void recycle()
        {
            if(valid())
            {
                _pool.recycle(mut_move(*this));
            }
        }

        void get() const noexcept requires(! has_res)
        {
            DSK_ASSERT(valid());
        }

        auto& get(this auto& self) noexcept requires(has_res)
        {
            DSK_ASSERT(self.valid());
            return *(self._it);
        }

        auto& operator*(this auto& self) noexcept requires(has_res)
        {
            return self.get();
        }

        auto* operator->(this auto& self) noexcept requires(has_res)
        {
            DSK_ASSERT(self.valid());
            return self._it.operator->();
        }
    };

    class async_acquire_op
    {
    public:
        res_pool*                              _pool = nullptr;
        std::optional<expected<res_ref, errc>> _r;
        any_resumer                            _resumer;
        continuation                           _cont;
        optional_stop_callback                 _scb; // must be last one defined

        explicit async_acquire_op(res_pool* p) : _pool(p) {}

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
                lock_guard lg(_pool->_mtx);

                if(auto r = _pool->try_acquire<false>())
                {
                    _r.emplace(mut_move(r));
                    return false;
                }

                _resumer = get_resumer(ctx);
                _cont = DSK_FORWARD(cont);

                if(stop_possible(ctx))
                {
                    _scb.emplace(get_stop_token(ctx), [this]()
                    {
                        _pool->cancel(this);
                    });
                }

                _pool->add_waiter_no_lock(this); // when initiate() gets called, this op should be
            }                                    // in its final place, so its address shouldn't change.

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

        // when invoked, this op should have been removed from pool, but still on same address,
        // so request_stop() won't affect result at this point.
        void complete(res_ref rr) // as _cont may be resumed inside, don't use res_ref&&
        {
            DSK_ASSERT(! _r);
            DSK_ASSERT(rr.valid());

            //_scb.reset();
            _r.emplace(expect, mut_move(rr));
            resume(mut_move(_cont), _resumer);
        }

        void complete_on_cancellation()
        {
            DSK_ASSERT(! _r);

            //_scb.reset();
            _r.emplace(unexpect, errc::canceled);
            resume(mut_move(_cont), _resumer);
        }
    };

    std::conditional_t<lock_outside, mutex&, mutex> _mtx;

    size_t                               _cap = 1;
    DSK_DEF_MEMBER_IF(! has_res, size_t) _size{0}; // size of in use for _void_<T>
    DSK_DEF_MEMBER_IF(  has_res, list_t) _inuse;
    DSK_DEF_MEMBER_IF(  has_res, list_t) _unused;

    DSK_DEF_MEMBER_IF(has_res,  Creator) _creator;
    DSK_DEF_MEMBER_IF(has_res, Recycler) _recycler;

    deque<async_acquire_op*> _waiterQueue; // ops waiting for res

    // unstable_unordered_map<
    //     async_acquire_op*,
    //     async_acquire_op**
    // > _waiters; // map to _waiterQueue item.

    res_iter invalid_iter() noexcept
    {
        if constexpr(has_res) return _inuse.end();
        else                  return 0;
    }

    void add_waiter_no_lock(async_acquire_op* w)
    {
        DSK_ASSERT(w);
        /*auto& b =*/ _waiterQueue.emplace_back(w);
        //DSK_VERIFY(_waiters.emplace(w, &b).second);
    }

    void cancel(async_acquire_op* w)
    {
        lock_guard lg(_mtx);

        //if(auto it = _waiters.find(w); it != _waiters.end())
        //{
        //    *(it->second) = nullptr; // mark _waiterQueue item as deleted
        //    _waiters.erase(it);
        //    w->complete_on_cancellation();
        //}

        // linear search. _waiterQueue should be relatively small
        if(auto it = std::ranges::find(_waiterQueue, w); it != _waiterQueue.end())
        {
            *it = nullptr; // mark as deleted
            w->complete_on_cancellation();
        }
    }

    void recycle(res_ref rr)
    {
        DSK_ASSERT(rr.valid());

        if constexpr(has_res)
        {
            _recycler(rr.get());
        }

        async_acquire_op* w = nullptr;

        {
            lock_guard lg(_mtx);

            while(_waiterQueue.size() && ! _waiterQueue.front())
            {
                _waiterQueue.pop_front(); // pop item marked as deleted
            }

            if(_waiterQueue.empty()) // no waiter, put back resource.
            {
                auto it = rr.release();

                if constexpr(has_res)
                {
                    _unused.splice(_unused.begin(), _inuse, it);
                }
                else
                {
                    DSK_ASSERT(_size > 0);
                    --_size;
                }

                return;
            }

            w = _waiterQueue.front();
            //DSK_VERIFY(_waiters.erase(w));
            _waiterQueue.pop_front();
        }

        //if constexpr(has_res)
        //    _onAcq(rr.get());

        w->complete(mut_move(rr));
    }

    size_t occupied_cap_nolock() const noexcept
    {
        if constexpr(has_res) return _unused.size() + _inuse.size();
        else                  return _size;
    }

public:
    res_pool(mutex& m, size_t cap, auto&& creator, auto&& recycler) requires(lock_outside)
        : _mtx(m), _cap(cap), _creator(DSK_FORWARD(creator)), _recycler(DSK_FORWARD(recycler))
    {
        DSK_ASSERT(_cap > 0);
    }

    explicit res_pool(size_t cap,  Creator const&  creator =  Creator(),
                                  Recycler const& recycler = Recycler()) requires(! lock_outside)
        : _cap(cap), _creator(creator), _recycler(recycler)
    {
        DSK_ASSERT(_cap > 0);
    }

    ~res_pool()
    {
        if constexpr(has_res) DSK_ASSERT(_inuse.empty() && _waiterQueue.empty() /*&& _waiters.empty()*/);
        else                  DSK_ASSERT(_size == 0 && _waiterQueue.empty() /*&& _waiters.empty()*/);
    }

    void set_creator(auto&& f) requires(has_res)
    {
        lock_guard lg(_mtx);
        _creator = DSK_FORWARD(f);
    }

    void set_recycler(auto&& f) requires(has_res)
    {
        lock_guard lg(_mtx);
        _recycler = DSK_FORWARD(f);
    }

    size_t created_count() const requires(has_res)
    {
        lock_guard lg(_mtx);
        return _unused.size() + _inuse.size();
    }

    size_t in_use_count() const
    {
        lock_guard lg(_mtx);

        if constexpr(has_res) return _inuse.size;
        else                  return _size;
    }

    size_t unused_count() const
    {
        lock_guard lg(_mtx);

        if constexpr(has_res) return _unused.size();
        else                  return _cap - _size;
    }

    size_t capacity() const
    {
        lock_guard lg(_mtx);
        return _cap;
    }

    void set_capacity(size_t n)
    {
        lock_guard lg(_mtx);
        DSK_ASSERT(n >= occupied_cap_nolock());
        _cap = n;            
    }

    void reserve(size_t n)
    {
        lock_guard lg(_mtx);

        if(n > occupied_cap_nolock())
        {
            _cap = n;
        }
    }

    void reserve_by(double ratio, size_t maxCap = SIZE_MAX)
    {
        DSK_ASSERT(ratio > 0);
        
        lock_guard lg(_mtx);

        size_t newCap = std::min(maxCap, static_cast<size_t>(std::floor(_cap * ratio)));

        if(newCap > occupied_cap_nolock())
        {
            _cap = newCap;
        }
    }

    // whether you can clear unused when running depends on the resource type and your use case.
    void clear_unused() requires(has_res)
    {
        lock_guard lg(_mtx);
        _unused.clear();
    }

    void create_all() requires(has_res)
    {
        lock_guard lg(_mtx);

        DSK_ASSERT(_cap >= occupied_cap_nolock());

        for(size_t n = _cap - occupied_cap_nolock(); n-- > 0;)
        {
            _creator(res_front_emplacer(_unused));
        }
    }

    template<bool Lock = true>
    expected<res_ref, errc> try_acquire()
    {
        conditional_lock_guard<Lock> lg(_mtx);

        if constexpr(has_res)
        {
            if(_unused.empty())
            {
                if(_inuse.size() < _cap) _creator(res_front_emplacer(_unused));
                else                     return errc::resource_unavailable;
            }

            _inuse.splice(_inuse.begin(), _unused, _unused.begin());

            //_onAcq(*_inuse.begin());

            return res_ref(*this, _inuse.begin());
        }
        else
        {
            if(_size < _cap) return res_ref(*this, ++_size);
            else             return errc::resource_unavailable;
        }
    }

    auto acquire()
    {
        return async_acquire_op(this);
    }
};


template<
    class Key,
    class T,
    class Creator  = default_res_creator,
    class Recycler = null_op_t,
    res_pool_map_add_pool_policy AutoAddPool = res_pool_map_auto_add_pool,
    res_pool_map_lock_policy     LockPolicy = res_pool_map_single_lock
>
class res_pool_map
{
public:
    using key_type  = Key;
    using pool_type = res_pool<T, Creator, Recycler, LockPolicy>;
    using res_ref   = pool_type::res_ref;

private:
    static constexpr bool has_res = ! _void_<T>;
    static constexpr bool lock_per_pool = (LockPolicy == res_pool_map_lock_per_pool);
    static_assert(is_oneof(LockPolicy, res_pool_map_single_lock, res_pool_map_lock_per_pool));

    mutex                                _mtx;
    stable_unordered_map<Key, pool_type> _pools;

    // defaults when creating new pool
    size_t                               _cap = 1;
    DSK_DEF_MEMBER_IF(has_res, Creator ) _creator;
    DSK_DEF_MEMBER_IF(has_res, Recycler) _recycler;

    struct rpm_async_acquire_op : pool_type::async_acquire_op
    {
        res_pool_map& _rpm;
        key_type _k;

        rpm_async_acquire_op(res_pool_map& m, auto&& k)
            : pool_type::async_acquire_op(nullptr), _rpm(m), _k(DSK_FORWARD(k))
        {}

        bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
        {
            DSK_ASSERT(! this->_r);

            if(stop_requested(ctx))
            {
                this->_r.emplace(unexpect, errc::canceled);
                return false;
            }

            {
                unique_lock lk(_rpm._mtx);

                if constexpr(AutoAddPool)
                {
                    this->_pool = _rpm.try_emplace_pool<false>(_k).first;
                }
                else
                {
                    this->_pool = _rpm.get_pool<false>(_k);

                    if(! this->_pool)
                    {
                        this->_r.emplace(unexpect, errc::not_found);
                        return false;
                    }
                }

                if constexpr(lock_per_pool)
                {
                    lk.unlock();
                }

                conditional_lock_guard<lock_per_pool> lg(this->_pool->_mtx);

                if(auto r = this->_pool->template try_acquire<false>())
                {
                    this->_r.emplace(mut_move(r));
                    return false;
                }

                this->_resumer = get_resumer(ctx);
                this->_cont = DSK_FORWARD(cont);

                if(stop_possible(ctx))
                {
                    this->_scb.emplace(get_stop_token(ctx), [this]()
                    {
                        this->_pool->cancel(this);
                    });
                }

                this->_pool->add_waiter_no_lock(this);
            }

            return true;
        }
    };

public:
    explicit res_pool_map(size_t cap, Creator const& creator = Creator(),
                                      Recycler const& recycler = Recycler())
        : _cap(cap), _creator(creator), _recycler(recycler)
    {
        DSK_ASSERT(_cap > 0);
    }

    void set_default_capacity(size_t n)
    {
        DSK_ASSERT(n > 0);
        lock_guard lg(_mtx);
        _cap = n;
    }

    void set_default_creator(auto&& f) requires(has_res)
    {
        lock_guard lg(_mtx);
        _creator = DSK_FORWARD(f);
    }

    void set_default_recycler(auto&& f) requires(has_res)
    {
        lock_guard lg(_mtx);
        _recycler = DSK_FORWARD(f);
    }

    template<bool Lock = true>
    pool_type* get_pool(auto const& k)
    {
        conditional_lock_guard<Lock> lg(_mtx);

        auto it = _pools.find(k);

        if(it != _pools.end()) return & it->second;
        else                   return nullptr;
    }

    template<bool Lock = true>
    std::pair<pool_type*, bool> try_emplace_pool(auto&& k, auto&&... args)
    {
        static_assert(sizeof...(args) <= 3);

        conditional_lock_guard<Lock> lg(_mtx);

        auto[it, ok] = apply_elms
        (
            tuple_cat
            (
                fwd_as_tuple_if<! lock_per_pool>(_mtx),
                fwd_as_tuple(DSK_FORWARD(args)...),
                select_last_n_elms<3 - sizeof...(args)>(fwd_as_tuple(_cap, _creator, _recycler))
            ),
            [&](auto&&... args)
            {
                return _pools.try_emplace(DSK_FORWARD(k), DSK_FORWARD(args)...);
            }
        );

        return {& it->second, ok};
    }

    expected<res_ref, errc> try_acquire(auto&& k)
    {
        unique_lock lk(_mtx);

        pool_type* p = nullptr;

        if constexpr(AutoAddPool)
        {
            p = try_emplace_pool<false>(DSK_FORWARD(k)).first;
        }
        else
        {
            p = get_pool<false>(k);

            if(! p)
            {
                return errc::not_found;
            }
        }

        if constexpr(lock_per_pool)
        {
            lk.unlock();
        }

        return p->template try_acquire<lock_per_pool>();
    }

    auto acquire(auto&& k)
    {
        return rpm_async_acquire_op(*this, DSK_FORWARD(k));
    }
};


} // namespace dsk
