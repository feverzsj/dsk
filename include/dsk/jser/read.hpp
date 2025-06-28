#pragma once

#include <dsk/jser/err.hpp>
#include <dsk/jser/cpo.hpp>
#include <dsk/util/ct.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/variant.hpp>
#include <dsk/optional.hpp>
#include <ranges>


namespace dsk{


/// read jval

template<size_t M = 0, bool Exact = false>
constexpr auto read_jval(auto&& p, auto& jv, auto& v)
{
    if constexpr(_arithmetic_<decltype(v)>)
    {
        return get_jarithmetic(jv, v);
    }
    else if constexpr(_str_<decltype(v)>)
    {
        return get_jstr(jv, v);
    }
    else if constexpr(_nullptr_<decltype(v)>)
    {
        return errc(); // do nothing if target is nullptr, can be used to skip reading array element.
    }
    else if constexpr(_tuple_like_<decltype(v)>)
    {
        return read_jtuple<M, Exact>(p, jv, v);
    }
    else if constexpr(std::ranges::range<decltype(v)>)
    {
        return read_jarr_of(p, jv, v);
    }
    else if constexpr(_optional_<decltype(v)>)
    return [&]() -> error_code
    {
        static_assert(_str_<decltype(*v)> || _arithmetic_<decltype(*v)>);

        DSK_U_TRY_FWD(null, check_jnull(jv));

        if(! null)
        {
            if(! v)
                v.emplace();

            return read_jval(p, jv, *v);
        }

        v.reset();
        return {};
    }();
    else if constexpr(_variant_<decltype(v)>)
    return [&]() -> error_code
    {
        static_assert(_same_as_<std::monostate, std::variant_alternative_t<0, DSK_NO_CVREF_T(v)>>);

        DSK_E_TRY_FWD(i, get_jval_matched_type(jv, DSK_TYPE_LIST(v)));

        return assure_var_holds(v, i, [&]<size_t I>(auto& fv) -> error_code
        {
            if constexpr(I) return read_jval(p, jv, fv);
            else            return {};
        });
    }();
    else
    {
        static_assert(false, "unsupported type");
    }
}

template<size_t M = 0, bool Exact = false>
constexpr auto read_jval(auto&& p, auto& jv, auto& d, auto& v)
{
    if constexpr(requires{ v(p, jv, d); }) return v(p, jv, d);
    else                                   return read_jval<M, Exact>(p, jv, jaccess_val(d, v));
}

/// read jobj

constexpr error_code read_jobj(auto&& p, auto& jv, auto& d, auto&& kvs)
{
    static_assert(kvs.count >= 2);
    static_assert(kvs.count % 2 == 0);

    auto[ks, nestedKs, vs, nestedVs] = partition_elms
    (
        kvs,
        DSK_TYPE_IDX_EXP(I%2 == 0 && ! _jarg_nested_<T>),
        DSK_TYPE_IDX_EXP(I%2 == 0),
        DSK_IDX_LIST_EXP(! _jarg_nested_<type_list_elm<I - 1, L>>)
    );

    static_assert(ks.count == vs.count);
    static_assert(nestedKs.count == nestedVs.count);
    static_assert((ks.count + nestedKs.count)*2 == kvs.count);

    DSK_U_TRY_FWD(finder, get_jval_finder(jv));

    if constexpr(ks.count)
    {
        auto mks = transform_elms(ks, DSK_ELM_EXP(tuple<bool, decltype(e)>{false, e}));

        DSK_E_TRY_FWD(unmatched, foreach_found_jval(finder, mks, [&]<auto I>(auto& fv)
        {
            return read_jval(p, fv, d, vs[idx_c<I>]);
        }));

        if(unmatched)
        {
            constexpr auto optIndices = ct_keep(DSK_TYPE_LIST(vs), [&]<class V>()
            {
                // clang gives wrong result or ICE
                //return requires(V v)
                //{
                //    requires requires{ v(p, jarg_no_fld(), d, jarg_no_fld()); }
                //             || _optional_<decltype(jaccess_val(d, v))>
                //             || _variant_<decltype(jaccess_val(d, v))>
                //             || _clearable_range_<decltype(jaccess_val(d, v))>;
                //};

                return requires{ std::declval<V&>()(p, jarg_no_fld(), d, jarg_no_fld()); }
                       || _optional_<decltype(jaccess_val(d, std::declval<V&>()))>
                       || _variant_<decltype(jaccess_val(d, std::declval<V&>()))>
                       || _clearable_range_<decltype(jaccess_val(d, std::declval<V&>()))>;
            });

            if constexpr(std::size(optIndices))
            {
                foreach_elms<optIndices>(mks, [&]<auto I>(auto& mk)
                {
                    if(! get_elm<0>(mk))
                    {
                        auto& v = vs[idx_c<I>];

                        if constexpr(requires{ v(p, jarg_no_fld(), d, jarg_no_fld()); })
                        {
                            DSK_U_TRY_ONLY(v(p, jarg_no_fld(), d, jarg_no_fld()));
                        }
                        else if constexpr(_optional_<decltype(jaccess_val(d, v))>)
                        {
                            auto& t = jaccess_val(d, v);
                            static_assert(_str_<decltype(*t)> || _arithmetic_<decltype(*t)>);
                            t.reset();
                        }
                        else if constexpr(_variant_<decltype(jaccess_val(d, v))>)
                        {
                            auto& t = jaccess_val(d, v);
                            static_assert(_same_as_<std::monostate, std::variant_alternative_t<0, DSK_NO_CVREF_T(t)>>);
                            t.template emplace<0>();
                        }
                        else if constexpr(_clearable_range_<decltype(jaccess_val(d, v))>)
                        {
                            auto& t = jaccess_val(d, v);
                            static_assert(_str_<unchecked_range_value_t<decltype(t)>> || _arithmetic_<unchecked_range_value_t<decltype(t)>>);
                            clear_range(t);
                        }

                        --unmatched;
                    }
                });

                if(unmatched)
                {
                    return jerrc::field_not_found;
                }
            }
            else
            {
                return jerrc::field_not_found;
            }
        }
    }

    if constexpr(nestedVs.count)
    {
        DSK_E_TRY_ONLY(foreach_elms_until_err(nestedVs, [&](auto& nestedV) -> error_code
        {
            return nestedV(p, finder, d, jarg_nested());
        }));
    }

    DSK_U_TRY_ONLY(finder.skip_remains());
    return {};
}


/// read jarr

template<size_t M = 0, bool Exact = false>
constexpr error_code read_jarr_of(auto&& p, auto& jv, std::ranges::range auto& d, auto&... ev)
{
    static_assert(sizeof...(ev) <= 1);

    DSK_U_TRY_FWD(it, get_jarr_iter(jv));

    using elm_type = std::ranges::range_value_t<decltype(d)>;

    if constexpr(requires{ d.push_back(std::declval<elm_type>()); })
    {
        d.clear();

        for(; it; ++it)
        {
            DSK_U_TRY_FWD(je, *it);

            elm_type ed;

            DSK_E_TRY_ONLY(read_jval<M, Exact>(p, je, ed, ev...));

            d.push_back(mut_move(ed));
        }
    }
    else // fix size range
    {
        for(auto& ed : d)
        {
            if(it)
            {
                DSK_U_TRY_FWD(je, *it);
                DSK_E_TRY_ONLY(read_jval<M, Exact>(p, je, ed, ev...));
                ++it;
            }
            else
            {
                return jerrc::out_of_bound;
            }
        }

        if constexpr(! requires{ typename DSK_NO_CVREF_T(it)::is_jpacked_arr_iter; }) // packed arr is not skipped here
        {
            DSK_U_TRY_ONLY(it.skip_remains());
        }
    }

    return {};
}

template<class T, size_t M = 0, bool Exact = false>
constexpr error_code read_jpacked_arr_of(auto&& p, auto& jv, std::ranges::range auto& d, auto&... ev)
{
    static_assert(sizeof...(ev) <= 1);

    if constexpr(requires{ get_jpacked_arr<T>(jv); })
    {
        DSK_E_TRY_FWD(jp, get_jpacked_arr<T>(jv));

        using vt = std::ranges::range_value_t<decltype(d)>;

        constexpr auto vts = uelms_to_type_list<vt>();
        constexpr auto m = M ? M : vts.count;

        static_assert(m <= vts.count);

        if constexpr(   sizeof...(ev) == 0
                     && m == vts.count
                     && ct_all_of(vts, type_c<T>)
                     && _buf_<decltype(d)>
                     && requires{ jp.memcpy_to(std::span<T>()); })
        {
            static_assert(sizeof(vt) == m * sizeof(T));
            static_assert(_trivially_copyable_<vt>);

            size_t const n = jp.count();

            if constexpr(m > 1)
            {
                if(n % m != 0)
                {
                    return jerrc::invalid_sub_elm_cnt;
                }
            }

            if constexpr(_resizable_buf_<decltype(d)>)
            {
                resize_buf(d, n / m);
            }
            else
            {
                if(buf_size(d) * m > n)
                {
                    return jerrc::invalid_elm_cnt;
                }
            }

            DSK_E_TRY_ONLY(jp.memcpy_to(as_buf_of<T>(d)));
        }
        else
        {
            DSK_E_TRY_ONLY(read_jarr_of<M, Exact>(p, jp, d, ev...));
        }

        return jp.skip_arr_remains();
    }
    else
    {
        return read_jarr_of<M, Exact>(p, jv, d, ev...);
    }
}

constexpr error_code read_elm_from_jarr_iter(auto&& p, auto& it, auto& d, auto&... v)
{
    static_assert(sizeof...(v) <= 1);

    if(it)
    {
        DSK_U_TRY_FWD(jv, *it);
        DSK_E_TRY_ONLY(read_jval(p, jv, d, v...));
        ++it;
        return {};
    }

    return jerrc::out_of_bound;
}

template<size_t M, bool Exact>
constexpr error_code read_jarr(auto&& p, auto& jv, auto& d, auto&... vs)
{
    if constexpr(sizeof...(vs))
    {
        static_assert(! M || M == sizeof...(vs));

        DSK_U_TRY_FWD(it, get_jarr_iter(jv));

        DSK_E_TRY_ONLY(foreach_elms_until_err(fwd_as_tuple(vs...), [&](auto& v)
        {
            return read_elm_from_jarr_iter(p, it, d, v);
        }));

        if constexpr(Exact)
        {
            if(it)
            {
                return jerrc::invalid_elm_cnt;
            }
        }

        DSK_U_TRY_ONLY(it.skip_remains());
        return {};
    }
    else
    {
        static_assert(_tuple_like_<decltype(d)>);

        return read_jtuple<M, Exact>(p, jv, d);
    }
}

template<size_t M, bool Exact>
constexpr error_code read_jtuple(auto&& p, auto& jv, _tuple_like_ auto& d, auto&... vs)
{
    constexpr size_t elmCnt = elm_count<decltype(d)>;

    DSK_U_TRY_FWD(it, get_jarr_iter(jv));

    if constexpr(sizeof...(vs))
    {
        static_assert(! M || M == sizeof...(vs));
        static_assert(sizeof...(vs) <= elmCnt);

        DSK_E_TRY_ONLY(foreach_elms_until_err(fwd_as_tuple(vs...), [&]<auto I>(auto& v)
        {
            return read_elm_from_jarr_iter(p, it, get_elm<I>(d), v);
        }));
    }
    else
    {
        static_assert(M <= elmCnt);

        constexpr size_t m = M ? M : elmCnt;

        DSK_E_TRY_ONLY(foreach_elms_until_err<make_index_array<m>()>(d, [&](auto& v)
        {
            return read_elm_from_jarr_iter(p, it, v);
        }));
    }

    if constexpr(Exact)
    {
        if(it)
        {
            return jerrc::invalid_elm_cnt;
        }
    }

    if constexpr(! requires{ typename DSK_NO_CVREF_T(it)::is_jpacked_arr_iter; }) // packed arr is not skipped here
    {
        DSK_U_TRY_ONLY(it.skip_remains());
    }

    return {};
}


} // namespace dsk
