#pragma once

#include <dsk/async_op.hpp>
#include <dsk/awaitables.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/list.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <optional>


namespace dsk{


// once until_all_done() is initiated, wait for all ops to finish, results are ignored.
// each op is initiated when add to group.
template<_async_ctx_ Ctx, class ResultHanlder = null_op_t>
class async_op_group
{
    struct data_t
    {;
        DSK_NO_UNIQUE_ADDR Ctx           ctx;
        DSK_NO_UNIQUE_ADDR ResultHanlder rh{};
        mutex                            mtx{};
        bool                             started = false;
        errc                             err{};
        continuation                     cont{};
        list<unique_function<void(data_t*, void*)>> ops{};
    };

    default_allocated_unique_ptr<data_t> _d;

public:
    explicit async_op_group(_async_ctx_ auto&& ctx)
        : _d(default_allocate_unique<data_t>(DSK_FORWARD(ctx)))
    {}

    async_op_group(_async_ctx_ auto&& ctx, auto&& rh)
        : _d(default_allocate_unique<data_t>(DSK_FORWARD(ctx), DSK_FORWARD(rh)))
    {}

    async_op_group(async_op_group&&) = default;
    async_op_group& operator=(async_op_group&&) = default;

    ~async_op_group()
    {
        // must wait until all done, as added ops were initiated.
        DSK_ASSERT(! _d || _d->ops.empty());
    }

    // must set before adding any op
    void set_result_handler(auto&& rh)
    {
        _d->rh = DSK_FORWARD(rh);
    }

    auto size() const noexcept
    {
        lock_guard lg(_d->mtx);
        return _d->ops.size();
    }

    void add_and_initiate(_async_op_ auto&& op)
    {
        auto it = [&]()
        {
            lock_guard lg(_d->mtx);

            DSK_ASSERT(! _d->cont);
            DSK_ASSERT(! _d->started);

            _d->ops.emplace_front([op = DSK_FORWARD(op)](data_t* d, void* itp) mutable
            {
                manual_initiate(op, d->ctx, [&op, d, it = *static_cast<decltype(_d->ops)::iterator*>(itp)]()
                {
                    d->rh(op);

                    bool done = [&]() mutable
                    {
                        lock_guard lg(d->mtx);

                        if(is_failed(op))
                        {
                            d->err = errc::one_or_more_ops_failed;
                        }

                        d->ops.erase(it);

                        return d->started && d->ops.empty();
                    }();

                    if(done)
                    {
                        //resume(mut_move(d->cont), d->ctx);
                        mut_move(d->cont)();
                    }
                });
            });

            return _d->ops.begin();
        }();

        (*it)(_d.get(), std::addressof(it));
    }

    struct [[nodiscard]] until_all_done_op
    {
        data_t* d = nullptr;

        using is_async_op = void;

        bool initiate(_continuation_ auto&& c)
        {
            // no cancel check, as added ops were initiated
            // and must be waited until all done.

            {
                lock_guard lg(d->mtx);

                if(d->ops.empty())
                {
                    return false;
                }

                DSK_ASSERT(! d->cont);
                DSK_ASSERT(! d->started);

                d->cont = DSK_FORWARD(c);
                d->started = true;
            }

            return true;
        }

        bool is_failed() const noexcept { return has_err(d->err); }
        errc take_result() noexcept { return d->err; }
    };

    auto until_all_done()
    {
        return until_all_done_op(_d.get());
    }
};

template<class C         > async_op_group(C   ) -> async_op_group<C   >;
template<class C, class R> async_op_group(C, R) -> async_op_group<C, R>;

// Create an async_op_group using current _async_ctx_.
// NOTE: async_op_group::until_all_done() will be added to current cleanup scope.
//       Its lifetime must match the scope.
template<class ResultHandler = null_op_t>
auto make_async_op_group(ResultHandler&& rh = ResultHandler{})
{
    return invoke_with_async_ctx([rh = DSK_FORWARD(rh)](_async_ctx_ auto&& ctx) mutable
    {
        async_op_group grp(ctx, mut_move(rh));
        add_cleanup(ctx, grp.until_all_done());
        return grp;
    });
}


template<_async_ctx_ Ctx, class ResultHanlder = null_op_t, errc Err = errc::one_or_more_ops_failed>
class lazy_async_op_group
{
    DSK_NO_UNIQUE_ADDR Ctx           _ctx;
    DSK_NO_UNIQUE_ADDR ResultHanlder _rh;
    atomic<int>                      _n = 1;
    atomic<errc>                     _err{};
    continuation                     _cont;
    deque<unique_function<void(lazy_async_op_group&)>> _ops;

public:
    explicit lazy_async_op_group(_async_ctx_ auto&& ctx) : _ctx(DSK_FORWARD(ctx)) {}
    lazy_async_op_group(_async_ctx_ auto&& ctx, auto&& rh) : _ctx(DSK_FORWARD(ctx)), _rh(DSK_FORWARD(rh)) {}

    void set_result_handler(auto&& rh) { _rh = DSK_FORWARD(rh); }
    void set_async_ctx(auto&& ctx) { _ctx = DSK_FORWARD(ctx); }

    auto  size() const noexcept { return _ops. size(); }
    bool empty() const noexcept { return _ops.empty(); }

    void add(_async_op_ auto&& op)
    {
        DSK_ASSERT(! _cont);
        DSK_ASSERT(_n.load(memory_order_relaxed) == 1);

        _ops.emplace_back([op = DSK_FORWARD(op)](lazy_async_op_group& g) mutable
        {
            manual_initiate(op, g._ctx, [&]() mutable
            {
                g._rh(op);

                if(is_failed(op))
                {
                    g._err.store(Err, memory_order_relaxed);
                }

                if(g._n.fetch_add(1, memory_order_acq_rel) == static_cast<int>(g._ops.size()))
                {
                    //resume(mut_move(g._cont), g._ctx);
                    mut_move(g._cont)();
                }
            });
        });
    }

    struct [[nodiscard]] until_all_done_op
    {
        lazy_async_op_group& _g;

        using is_async_op = void;

        bool initiate(_continuation_ auto&& cont)
        {
            DSK_ASSERT(! _g._cont);
            DSK_ASSERT(! has_err(_g._err.load(memory_order_relaxed)));
            DSK_ASSERT(_g._n.load(memory_order_relaxed) == 1);

            if(stop_requested(_g._ctx))
            {
                _g._err.store(errc::canceled, memory_order_relaxed);
                return false;
            }

            if(_g._ops.empty())
            {
                return false;
            }

            _g._cont = DSK_FORWARD(cont);

            // NOTE: when last op is called, the continuation may be called concurrently,
            //       which may causing _g being destroyed, so we need cache the size
            //       to avoid using _g after last op.
            for(size_t i = 0, n = _g._ops.size(); i < n; ++i)
            {
                _g._ops[i](_g);
            }

            return true;
        }

        bool is_failed() const noexcept
        {
            _g._ops.clear();
            return has_err(_g._err.load(memory_order_relaxed));
        }
        
        errc take_result() noexcept
        {
            _g._ops.clear();
            return _g._err.load(memory_order_relaxed);
        }
    };

    auto until_all_done()
    {
        return until_all_done_op(*this);
    }
};

template<class C         > lazy_async_op_group(C   ) -> lazy_async_op_group<C   >;
template<class C, class R> lazy_async_op_group(C, R) -> lazy_async_op_group<C, R>;

template<class ResultHandler = null_op_t>
auto make_lazy_async_op_group(ResultHandler&& rh = ResultHandler{})
{
    return invoke_with_async_ctx([rh = DSK_FORWARD(rh)](_async_ctx_ auto&& ctx) mutable
    {
        return lazy_async_op_group(DSK_FORWARD(ctx), mut_move(rh));
    });
}


template<_async_ctx_ Ctx, class ResultHanlder = null_op_t, errc Err = errc::one_or_more_ops_failed>
class lazy_async_op_group_stack
{
    using op_grp_type = lazy_async_op_group<Ctx, ResultHanlder, Err>;

    DSK_NO_UNIQUE_ADDR Ctx           _ctx;
    DSK_NO_UNIQUE_ADDR ResultHanlder _rh;

    // typical deque implementation pre-allocates a block on constructing,
    // as we gonna put it in coro frame, we want to avoid extra allocation.
    std::optional<deque<op_grp_type>> _gps;

public:
    explicit lazy_async_op_group_stack(_async_ctx_ auto&& ctx) : _ctx(DSK_FORWARD(ctx)) {}
    lazy_async_op_group_stack(_async_ctx_ auto&& ctx, auto&& rh) : _ctx(DSK_FORWARD(ctx)), _rh(DSK_FORWARD(rh)) {}

    void set_result_handler(auto&& rh) { DSK_ASSERT(empty()); _rh = DSK_FORWARD(rh); }
    void set_async_ctx(auto&& ctx) { DSK_ASSERT(empty()); _ctx = DSK_FORWARD(ctx); }

    auto& top()       noexcept { DSK_ASSERT(size()); return _gps->back(); }
    auto& top() const noexcept { DSK_ASSERT(size()); return _gps->back(); }

    auto  size() const noexcept { return _gps ? _gps->size() : 0; }
    bool empty() const noexcept { return ! _gps || _gps->empty(); }

    auto& push() { return (_gps ? *_gps : _gps.emplace()).emplace_back(_ctx, _rh); }
    void   pop() { DSK_ASSERT(size()); _gps->pop_back(); }
    void clear() { if(_gps) _gps->clear(); }

    void pop_untill_non_empty()
    {
        while(size() && top().empty())
        {
            pop();
        }
    }

    void add(_async_op_ auto&& op)
    {
        top().add(DSK_FORWARD(op));
    }

    struct push_pop_guard
    {
        lazy_async_op_group_stack& _s;
        op_grp_type& _g;

        explicit push_pop_guard(lazy_async_op_group_stack& s)
            : _s(s), _g(s.push())
        {}

        ~push_pop_guard()
        {
            if(_s.size()) // if no throw/return inside guard scope, which triggers cleanup of the whole lazy_async_op_group_stack.
            {
                assert_top();
                DSK_RELEASE_ASSERT(_g.empty());
                _s.pop();
            }
        }

        void add(_async_op_ auto&& op)
        {
            //assert_top();
            _g.add(DSK_FORWARD(op));
        }

        auto until_all_done()
        {
            assert_top();
            return _g.until_all_done();
        }

        void assert_top()
        {
            DSK_ASSERT(std::addressof(_g) == std::addressof(_s.top()));
        }
    };

    auto make_push_pop_guard()
    {
        return push_pop_guard(*this);
    }

    struct [[nodiscard]] until_all_done_op
    {
        lazy_async_op_group_stack&                          _gs;
        errc                                                _err{};
        continuation                                        _cont;
        std::optional<decltype(_gs.top().until_all_done())> _gop;

        using is_async_op = void;

        bool initiate(_continuation_ auto&& cont)
        {
            if(set_canceled_if_stop_requested(_err, _gs._ctx))
            {
                //_gs.clear();
                return false;
            }

            _gs.pop_untill_non_empty();

            if(_gs.empty())
            {
                return false;
            }

            _cont = DSK_FORWARD(cont);

            _gop.emplace(_gs.top().until_all_done());

            return dsk::initiate(*_gop, _gs._ctx, [this](this auto&& cont) -> void // without explicit return type, _continuation_<decltype(cont)> will fail,
            {                                                                      // because to deduce the return type, cont need complete.
                if(dsk::is_failed(*_gop))
                {
                    _err = Err;
                }

                _gs.pop();
                _gs.pop_untill_non_empty();

                if(_gs.size())
                {
                    _gop.emplace(_gs.top().until_all_done());
                    manual_initiate(*_gop, _gs._ctx, mut_move(cont));
                }
                else
                {
                    //resume(mut_move(_cont), _gs._ctx);
                    mut_move(_cont)();
                }
            });
        }

        bool is_failed() const noexcept { return has_err(_err); }
        errc take_result() noexcept { return _err; }
    };

    auto until_all_done()
    {
        return until_all_done_op(*this);
    }
};

template<class C         > lazy_async_op_group_stack(C   ) -> lazy_async_op_group_stack<C   >;
template<class C, class R> lazy_async_op_group_stack(C, R) -> lazy_async_op_group_stack<C, R>;

template<class ResultHandler = null_op_t>
auto make_lazy_async_op_group_stack(ResultHandler&& rh = ResultHandler{})
{
    return invoke_with_async_ctx([rh = DSK_FORWARD(rh)](_async_ctx_ auto&& ctx) mutable
    {
        return lazy_async_op_group_stack(DSK_FORWARD(ctx), mut_move(rh));
    });
}


} // namespace dsk
