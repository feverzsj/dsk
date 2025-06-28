#pragma once

#include <dsk/jser/err.hpp>
#include <dsk/jser/cpo.hpp>
#include <dsk/optional.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/stringify.hpp>
#include <ranges>


namespace dsk{


// write jval

template<size_t M = 0>
constexpr void write_jval(auto&& p, _jwriter_ auto& w, auto const& v)
{
    if constexpr(_arithmetic_<decltype(v)>)
    {
        return w.arithmetic(v);
    }
    else if constexpr(_str_<decltype(v)>)
    {
        return w.str(v);
    }
    else if constexpr(_nullptr_<decltype(v)>)
    {
        w.null();
    }
    else if constexpr(_tuple_like_<decltype(v)>)
    {
        write_jtuple<M>(p, w, v);
    }
    else if constexpr(std::ranges::range<decltype(v)>)
    {
        write_jarr_of(p, w, v);
    }
    else if constexpr(_optional_<decltype(v)>)
    {
        static_assert(_str_<decltype(*v)> || _arithmetic_<decltype(*v)>);

        if(v) write_jval(p, w, *v);
        else  w.null();
    }
    else if constexpr(_variant_<decltype(v)>)
    {
        static_assert(_same_as_<std::monostate, std::variant_alternative_t<0, DSK_NO_CVREF_T(v)>>);

        visit_var(v, [&]<auto I>(auto& av)
        {
            if constexpr(I) write_jval(p, w, av);
            else            w.null();
        });
    }
    else
    {
        static_assert(false, "unsupported type");
    }
}

template<size_t M = 0>
constexpr void write_jval(auto&& p, _jwriter_ auto& w, auto const& d, auto&& v)
{
    if constexpr(requires{ v(p, w, d); }) v(p, w, d);
    else                                  write_jval<M>(p, w, jaccess_val(d, v));
}

// write jobj

constexpr size_t write_jflds(auto&& p, _jwriter_ auto& w, auto const& d, auto&& kvs)
{
    return apply_elms(kvs, [&](this auto&& self, auto const& k, auto&& v, auto&&... kvs) -> size_t
    {
        size_t cnt = 0;

        if constexpr(_jarg_nested_<decltype(k)>)
        {
            cnt = v(p, w, d, jarg_nested());
        }
        else if constexpr(requires{ v(p, w, d, jarg_key(k)); })
        {
            v(p, w, d, jarg_key(k));
            cnt = 1;
        }
        else if constexpr(_optional_<decltype(jaccess_val(d, v))>)
        {
            auto& t = jaccess_val(d, v);

            static_assert(_str_<decltype(*t)> || _arithmetic_<decltype(*t)>);

            if(t)
            {
                w.fld_scope(k, [&]()
                {
                    write_jval(p, w, *t);
                });

                cnt = 1;
            }
        }
        else if constexpr(_variant_<decltype(jaccess_val(d, v))>)
        {
            auto& t = jaccess_val(d, v);

            static_assert(_same_as_<std::monostate, std::variant_alternative_t<0, DSK_NO_CVREF_T(t)>>);

            if(t.index())
            {
                w.fld_scope(k, [&]()
                {
                    visit_var(t, [&]<auto I>(auto& av)
                    {
                        if constexpr(I) write_jval(p, w, av);
                    });
                });

                cnt = 1;
            }
        }
        else
        {
            w.fld_scope(k, [&]()
            {
                write_jval(p, w, d, v);
            });

            cnt = 1;
        }

        if constexpr(sizeof...(kvs))
        {
            cnt += self(kvs...);
        }

        return cnt;
    });
}


// write jarr

template<size_t M = 0>
constexpr void write_jarr_of(auto&& p, _jwriter_ auto& w, std::ranges::range auto const& d, auto&&... ev)
{
    static_assert(sizeof...(ev) <= 1);

    w.arr_scope(std::ranges::size(d), [&]()
    {
        for(auto& ed : d)
        {
            w.elm_scope([&]()
            {
                write_jval<M>(p, w, ed, ev...);
            });
        }
    });
}

template<class T, size_t M>
constexpr void write_jpacked_arr_of(auto&& p, _jwriter_ auto& w, std::ranges::range auto const& d, auto&&... ev)
{
    static_assert(sizeof...(ev) <= 1);

    if constexpr(requires{ w.template packed_arr_scope<T>(1, [&](auto&&){}); })
    {
        using vt = std::ranges::range_value_t<decltype(d)>;

        constexpr auto vts = uelms_to_type_list<vt>();
        constexpr auto m = M ? M : vts.count;

        static_assert(m <= vts.count);

        if constexpr(   sizeof...(ev) == 0
                     && m == vts.count
                     && ct_all_of(vts, type_c<T>)
                     && _buf_<decltype(d)>
                     && requires{ w.packed_arr_buf(std::span<T>()); })
        {
            static_assert(sizeof(vt) == m * sizeof(T));
            static_assert(_trivially_copyable_<vt>);

            w.packed_arr_buf(as_buf_of<T>(d));
        }
        else
        {
            w.template packed_arr_scope<T>(std::ranges::size(d) * m, [&](auto&& w) // overridden writer
            {
                for(auto& ed : d)
                {
                    write_jval<M>(p, w, ed, ev...);
                }
            });
        }
    }
    else
    {
        write_jarr_of<M>(p, w, d, ev...);
    }
}


template<size_t M>
constexpr void write_jarr(auto&& p, _jwriter_ auto& w, auto const& d, auto&&... vs)
{    
    if constexpr(sizeof...(vs))
    {
        static_assert(! M || M == sizeof...(vs));

        w.arr_scope(sizeof...(vs), [&]()
        {
            foreach_elms(fwd_as_tuple(vs...), [&](auto& v)
            {
                w.elm_scope([&]()
                {
                    write_jval(p, w, d, v);
                });
            });
        });
    }
    else
    {
        static_assert(_tuple_like_<decltype(d)>);

        write_jtuple<M>(p, w, d);
    }
}

template<size_t M>
constexpr void write_jtuple(auto&& p, _jwriter_ auto& w, _tuple_like_ auto const& d, auto&&... vs)
{
    constexpr size_t elmCnt = elm_count<decltype(d)>;
    
    if constexpr(sizeof...(vs))
    {
        static_assert(! M || M == sizeof...(vs));
        static_assert(sizeof...(vs) <= elmCnt);

        w.arr_scope(sizeof...(vs), [&]()
        {
            foreach_elms(fwd_as_tuple(vs...), [&]<auto I>(auto& v)
            {
                w.elm_scope([&]()
                {
                    write_jval(p, w, get_elm<I>(d), v);
                });
            });
        });
    }
    else
    {
        static_assert(M <= elmCnt);

        constexpr size_t m = M ? M : elmCnt;

        w.arr_scope(m, [&]()
        {
            foreach_elms<make_index_array<m>()>(d, [&](auto& v)
            {
                w.elm_scope([&]()
                {
                    write_jval(p, w, v);
                });
            });
        });
    }
}


} // namespace dsk
