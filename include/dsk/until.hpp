#pragma once

#include <dsk/async_op.hpp>
#include <dsk/util/ct.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/util/vector.hpp>


namespace dsk{


template<class T>
concept _async_op_tuple_or_range_ = (_tuple_like_<T> && ct_all_of(elms_to_type_list<T>(), DSK_TYPE_EXP_OF(E, _async_op_<E>)))
                                    || (std::ranges::range<T> && _async_op_<unchecked_range_value_t<T>>);


template<_async_op_tuple_or_range_ Ops>
struct [[nodiscard]] until_all_done_op
{
    static constexpr bool isTuple = _tuple_like_<Ops>;
    // 0 for empty tuple, 1 for single elm tuple, > 1 for tuple with nElm or range.
    static constexpr size_t nElm = DSK_CONDITIONAL_V_NO_CAPTURE(isTuple, elm_count<Ops>, 2);

    DSK_NO_UNIQUE_ADDR                   Ops  _ops;
                                        errc  _err{};
    DSK_DEF_MEMBER_IF(nElm > 1,          int) _n = std::size(_ops); // atomic_ref, as _n is only used after this op is on its final address, while atomic is not movable. 
    DSK_DEF_MEMBER_IF(nElm > 1, continuation) _cont;

    using is_async_op = void;

    bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        if(set_canceled_if_stop_requested(_err, ctx))
        {
            return false;
        }

        if constexpr(nElm == 1) // single elm tuple
        {
            manual_initiate(get_elm<0>(_ops), DSK_FORWARD(ctx), DSK_FORWARD(cont));
        }
        else if constexpr(nElm > 1) // tuple with nElm or range
        {
            if constexpr(! isTuple)
            {
                if(std::size(_ops) <= 1)
                {
                    if(std::size(_ops) == 1)
                    {
                        manual_initiate(*std::begin(_ops), DSK_FORWARD(ctx), DSK_FORWARD(cont));
                    }

                    return true;
                }
            }

            _cont = DSK_FORWARD(cont);

            uforeach(_ops, [&](auto& op)
            {
                manual_initiate(op, ctx, [this/*, ctx*/]() mutable
                {
                    if(atomic_ref(_n).fetch_sub(1, memory_order_acq_rel) == 1)
                    {
                        //resume(mut_move(_cont), ctx);
                        mut_move(_cont)();
                    }
                });
            });
        }

        return true;
    }

    constexpr bool is_failed() const noexcept
    {
        return has_err(_err);
    }

    // for tuple of ops, returns expected<tupe<result_type_of_ops...>, errc>
    // for range of ops, returns expected<vector<result_type_of_ops>, errc>
    constexpr decltype(auto) take_result()
    {
        return gen_expected_if_no(_err, [this]()
        {
            if constexpr(isTuple)
            {
                return transform_elms(_ops, DSK_ELM_EXP(take_result<voidness_as_void>(e)));
            }
            else
            {
                vector<decltype(take_result<voidness_as_void>(*std::begin(_ops)))> rs;
                rs.reserve(std::size(_ops));

                for(auto& op : _ops)
                {
                    rs.emplace_back(take_result<voidness_as_void>(op));
                }

                return rs;
            }
        });
    }
};


// Wait until all ops to finish.
auto until_all_done(_async_op_tuple_or_range_ auto&& ops)
{
    return make_templated<until_all_done_op>(DSK_FORWARD(ops));
}

// Result type: expected<tupe<result_type_of_ops...>, errc>
auto until_all_done(_async_op_ auto&&... ops)
{
    return until_all_done(tuple(DSK_FORWARD(ops)...));
}

// Result type: expected<tupe<result_type_of_ops...>, errc>
template<size_t N>
auto until_all_done(_nullary_callable_ auto&& genOp)
{
    auto indexedGen = [&](auto){ return genOp(); };

    return apply_ct_elms<make_index_array<N>()>([&]<auto... I>()
    {
        return until_all_done(indexedGen(I)...);
    });
}

// Result type: expected<vector<result_type_of_ops>, errc>
auto until_all_done(size_t n, _nullary_callable_ auto&& genOp)
{
    vector<decltype(genOp())> ops;
    ops.reserve(n);

    for(size_t i = 0; i < n; ++i)
    {
        ops.emplace_back(genOp());
    }

    return until_all_done(mut_move(ops));
}


enum until_first_e
{
    until_first_take_cat_mask         = 0xff,
    until_first_take_first_founded    = 1 << 8,
    until_first_take_all_if_not_found = 1 << 9,

    until_first_take_first_founded_result           = until_first_take_first_founded | result_cat,
    until_first_take_first_founded_result_value     = until_first_take_first_founded | result_value_cat,
    until_first_take_all_results_if_not_found       = until_first_take_all_if_not_found | result_cat,
    until_first_take_all_result_values_if_not_found = until_first_take_all_if_not_found | result_value_cat,
};

template<until_first_e Cat, class CancelCond, _async_op_tuple_or_range_ Ops>
struct [[nodiscard]] until_if_op
{
    static constexpr bool isTuple = _tuple_like_<Ops>;
    // 0 for empty tuple, 1 for single elm tuple, > 1 for tuple with nElm or range.
    static constexpr size_t nElm = DSK_CONDITIONAL_V_NO_CAPTURE(isTuple, elm_count<Ops>, 2);

    DSK_NO_UNIQUE_ADDR                      CancelCond  _cancelCond;
    DSK_NO_UNIQUE_ADDR                             Ops  _ops;
                                                  errc  _err{};
    DSK_DEF_MEMBER_IF(nElm > 0,                    int) _firstIdx = -1;
    DSK_DEF_MEMBER_IF(nElm > 1,                    int) _n = std::size(_ops); // atomic_ref, as _n is only used after this op is on its final address, while atomic is not movable. 
    DSK_DEF_MEMBER_IF(nElm > 1,           continuation) _cont;
    DSK_DEF_MEMBER_IF(nElm > 1,       std::stop_source) _opSs{std::nostopstate};
    DSK_DEF_MEMBER_IF(nElm > 1, optional_stop_callback) _scb; // must be last defined

    void set_err_from_first_idx()
    {
        if constexpr(test_flag(Cat, until_first_take_first_founded))
        {
            if(_firstIdx < 0)
            {
                _err = errc::not_found;
            }
        }
        else
        {
            static_assert(test_flag(Cat, until_first_take_all_if_not_found));

            if(_firstIdx >= 0)
            {
                _err = errc::failed;
            }
        }
    }

    using is_async_op = void;

    bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        if(set_canceled_if_stop_requested(_err, ctx))
        {
            return false;
        }

        if constexpr(nElm == 1) // single elm tuple
        {
            manual_initiate(get_elm<0>(_ops), DSK_FORWARD(ctx), [this, cont = DSK_FORWARD(cont)]() mutable
            {
                if(_cancelCond(get_elm<0>(_ops)))
                {
                    _firstIdx = 0;
                }

                set_err_from_first_idx();
                mut_move(cont)();
            });
        }
        else if constexpr(nElm > 1) // tuple with nElm or range
        {
            if constexpr(! isTuple)
            {
                if(std::size(_ops) <= 1)
                {
                    if(std::size(_ops) == 1)
                    {
                        manual_initiate(*std::begin(_ops), DSK_FORWARD(ctx), [this, cont = DSK_FORWARD(cont)]() mutable
                        {
                            if(_cancelCond(*std::begin(_ops)))
                            {
                                _firstIdx = 0;
                            }

                            set_err_from_first_idx();
                            mut_move(cont)();
                        });
                    }

                    return true;
                }
            }

            _cont = DSK_FORWARD(cont);
            _opSs = std::stop_source();

            // _cont may be immediately resumed in manual_initiate,
            // which destroy this object, so must prepare everything ahead.
            if(stop_possible(ctx))
            {
                _scb.emplace(get_stop_token(ctx), [this]()
                {
                    _opSs.request_stop();
                });
            }

            auto ssCtx = make_async_ctx(ctx, std::ref(_opSs));
            int idx = 0;

            uforeach(_ops, [&](auto& op)
            {
                manual_initiate(op, ssCtx, [this, &op, idx = idx++/*, ssCtx*/]() mutable
                {
                    if(_cancelCond(op))
                    {
                        if(_opSs.request_stop())
                        {
                            _firstIdx = idx;
                        }
                    }

                    // once ops are initialized, they should be waited until all finished
                    if(atomic_ref(_n).fetch_sub(1, memory_order_acq_rel) == 1)
                    {
                        set_err_from_first_idx();

                        //resume(mut_move(_cont), ssCtx);
                        mut_move(_cont)();
                    }
                });
            });
        }

        return true;
    }

    constexpr bool is_failed() const noexcept
    {
        return has_err(_err);
    }

    // result type:
    //    until_first_take_first_founded:
    //        expected<void, errc> for empty tuple.
    //        expected<std::varaint<result[_value]_types...>, errc> for non empty tuple, varaint.index() is the first found idx.
    //        expected<std::pair<result[_value]_type, size_t>, errc> for range, pair.second is the first found idx.
    //    until_first_take_all_if_not_found:
    //        expected<tupe<result[_value]_types...>, errc>.
    constexpr auto take_result()
    {
        constexpr auto takeCat = static_cast<take_cat_e>(Cat & until_first_take_cat_mask);

        if constexpr(test_flag(Cat, until_first_take_first_founded))
        {
            if constexpr(nElm == 0)
            {
                DSK_ASSERT(_firstIdx == -1);
                return make_expected_if_no(_err);
            }
            else
            {
                return gen_expected_if_no(_err, [&]() //-> var_t // clang workaround
                {
                    DSK_ASSERT(_firstIdx >= 0);

                    if constexpr(isTuple)
                    {
                        using var_t = typename decltype(
                            ct_transform<cvt_type_list_t<Ops, std::variant>>(DSK_TYPE_EXP(
                                type_c<decltype(take<takeCat, voidness_as_void>(std::declval<T>()))>))
                            )::type;

                        return visit_elm(_firstIdx, _ops, []<auto I>(auto& op)
                        {
                            return var_t(std::in_place_index<I>, take<takeCat, voidness_as_void>(op));
                        });
                    }
                    else
                    {
                        return std::pair(take<takeCat, voidness_as_void>(*std::next(std::begin(_ops), _firstIdx)),
                                         static_cast<size_t>(_firstIdx));
                    }
                });
            }
        }
        else
        {
            static_assert(test_flag(Cat, until_first_take_all_if_not_found));

            return gen_expected_if_no(_err, [&]()
            {
                DSK_ASSERT(_firstIdx == -1);

                if constexpr(isTuple)
                {
                    return transform_elms(_ops, DSK_ELM_EXP(take<takeCat, voidness_as_void>(e)));
                }
                else
                {
                    vector<decltype(take<takeCat, voidness_as_void>(*std::begin(_ops)))> rs;
                    rs.reserve(std::size(_ops));

                    for(auto& op : _ops)
                    {
                        rs.emplace_back(take<takeCat, voidness_as_void>(op));
                    }

                    return rs;
                }
            });
        }
    }
};


template<until_first_e Cat>
auto until_first(auto&& cancelCond, _async_op_tuple_or_range_ auto&& ops)
{
    return make_templated_nt1<until_if_op, Cat>(DSK_FORWARD(cancelCond), DSK_FORWARD(ops));
}

template<until_first_e Cat>
auto until_first(auto&& cancelCond, _async_op_ auto&&... ops)
{
    return until_first<Cat>(DSK_FORWARD(cancelCond), tuple(DSK_FORWARD(ops)...));
}

template<until_first_e Cat, size_t N>
auto until_first(auto&& cancelCond, _nullary_callable_ auto&& genOp)
{
    auto indexedGen = [&](auto){ return genOp(); };

    return apply_ct_elms<make_index_array<N>()>([&]<auto... I>()
    {
        return until_first<Cat>(DSK_FORWARD(cancelCond), indexedGen(I)...);
    });
}

template<until_first_e Cat>
auto until_first(size_t n, auto&& cancelCond, _nullary_callable_ auto&& genOp)
{
    vector<decltype(genOp())> ops;
    ops.reserve(n);

    for(size_t i = 0; i < n; ++i)
    {
        ops.emplace_back(genOp());
    }

    return until_first<Cat>(DSK_FORWARD(cancelCond), mut_move(ops));
}


// NOTE: all ops are waited until finished.


// Wait until first op to finish, then cancel unfinished ops.
auto until_first_done(_async_op_tuple_or_range_ auto&& ops)
{
    return until_first<until_first_take_first_founded_result>(
        [](auto&){ return true; }, DSK_FORWARD(ops));
}

auto until_first_done(_async_op_ auto&&... ops)
{
    return until_first<until_first_take_first_founded_result>(
        [](auto&){ return true; }, DSK_FORWARD(ops)...);
}

template<size_t N>
auto until_first_done(_nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result, N>(
        [](auto&){ return true; }, DSK_FORWARD(genOp));
}

auto until_first_done(size_t n, _nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result>(n,
        [](auto&){ return true; }, DSK_FORWARD(genOp));
}

// Wait until first op to finish successfully, then cancel unfinished ops.
// If no op finished successfully, return errc::not_found.
auto until_first_succeeded(_async_op_tuple_or_range_ auto&& ops)
{
    return until_first<until_first_take_first_founded_result_value>(
        [](auto& op){ return ! is_failed(op); }, DSK_FORWARD(ops));
}

auto until_first_succeeded(_async_op_ auto&&... ops)
{
    return until_first<until_first_take_first_founded_result_value>(
        [](auto& op){ return ! is_failed(op); }, DSK_FORWARD(ops)...);
}

template<size_t N>
auto until_first_succeeded(_nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result_value, N>(
        [](auto& op){ return ! is_failed(op); }, DSK_FORWARD(genOp));
}

auto until_first_succeeded(size_t n, _nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result_value>(n,
        [](auto& op){ return ! is_failed(op); }, DSK_FORWARD(genOp));
}

// Wait until first op to finish unsuccessfully, then cancel unfinished ops.
// If no op finished unsuccessfully, return errc::not_found.
auto until_first_failed(_async_op_tuple_or_range_ auto&& ops)
{
    return until_first<until_first_take_first_founded_result>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(ops));
}

auto until_first_failed(_async_op_ auto&&... ops)
{
    return until_first<until_first_take_first_founded_result>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(ops)...);
}

template<size_t N>
auto until_first_failed(_nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result, N>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(genOp));
}

auto until_first_failed(size_t n, _nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_first_founded_result>(n,
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(genOp));
}


// Wait until all ops to finish successfully.
// If any op failed, unfinished ops are canceled and errc::failed is returned.
// Result type: expected<tupe<result_value_type_of_ops...>>.
auto until_all_succeeded(_async_op_tuple_or_range_ auto&& ops)
{
    return until_first<until_first_take_all_result_values_if_not_found>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(ops));
}

auto until_all_succeeded(_async_op_ auto&&... ops)
{
    return until_first<until_first_take_all_result_values_if_not_found>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(ops)...);
}

template<size_t N>
auto until_all_succeeded(_nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_all_result_values_if_not_found, N>(
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(genOp));
}

auto until_all_succeeded(size_t n, _nullary_callable_ auto&& genOp)
{
    return until_first<until_first_take_all_result_values_if_not_found>(n,
        [](auto& op){ return is_failed(op); }, DSK_FORWARD(genOp));
}


// Until the op which cancelCond(op) is true, or the last op
// R is the result type.
// If R is blank_t, it will be deduced, so all op's results should be same. 
// If R is void, result is ignored.
template<class R, class CancelCond, _tuple_ Ops>
struct [[nodiscard]] seq_until_if_op
{
    static_assert(Ops::count);

    DSK_NO_UNIQUE_ADDR CancelCond _cancelCond;
    DSK_NO_UNIQUE_ADDR Ops        _ops;
                       size_t     _idx = 0;

    using is_async_op = void;

    void initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        DSK_ASSERT(_idx == 0);

        manual_initiate
        (
            _ops.first(), ctx,
            [this, ctx, cont = DSK_FORWARD(cont)](this auto&& self)
            {
                visit_elm(_idx, _ops, [&](auto& op)
                {
                    if(_idx + 1 == _ops.count || _cancelCond(op))
                    {
                        //resume(mut_move(cont), ctx);
                        mut_move(cont)();
                        return;
                    }

                    visit_elm(++_idx, _ops, [&](auto& op)
                    {
                        manual_initiate(op, ctx, mut_move(self));
                    });
                });
            }
        );
    }

    constexpr bool is_failed() const noexcept
    {
        return visit_elm(_idx, _ops, [&](auto& op)
        {
            return dsk::is_failed(op);
        });
    }

    constexpr decltype(auto) take_result()
    {
        if constexpr(_blank_<R>) // auto deduced, all result types should be compatible.
        {
            return visit_elm(_idx, _ops, [&](auto& op) -> decltype(auto)
            {
                return dsk::take_result(op);
            });
        }
        else if constexpr(! _void_<R>)
        {
            return visit_elm(_idx, _ops, [&](auto& op) -> R
            {
                return dsk::take_result(op);
            });
        }
    }
};


template<class R = blank_t>
auto seq_until(auto&& cancelCond, _async_op_ auto&&... ops)
{
    return make_templated<seq_until_if_op, R>(DSK_FORWARD(cancelCond), tuple(DSK_FORWARD(ops)...));
}

template<class R = blank_t>
auto seq_until_succeeded(_async_op_ auto&&... ops)
{
    return seq_until<R>([](auto& op){ return ! is_failed(op); }, DSK_FORWARD(ops)...);
}

template<class R = blank_t>
auto seq_until_failed(_async_op_ auto&&... ops)
{
    return seq_until<R>([](auto& op){ return is_failed(op); }, DSK_FORWARD(ops)...);
}


} // namespace dsk
