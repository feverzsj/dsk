#pragma once

#include <dsk/jser/read.hpp>
#include <dsk/jser/write.hpp>
#include <dsk/util/strlit.hpp>
#include <dsk/util/variant.hpp>
#include <dsk/util/from_str.hpp>


namespace dsk{


constexpr auto&& jsel(auto&& v) noexcept
{
    return DSK_FORWARD(v);
}

constexpr auto jsel(auto&& v, auto&& f)
{
    return [v = DSK_FORWARD(v), f = DSK_FORWARD(f)](auto&& p, auto&& j, auto&& d, auto&&... arg) -> decltype(auto)
    requires(
        requires{ f(p, j, jaccess_val(d, v), arg...); })
    {
        return f(p, j, jaccess_val(d, v), arg...);
    };
}

// Select member of current data.
#define DSK_JSEL(m, ...) jsel([](auto& d) -> auto& { return d.m; } __VA_OPT__(,) __VA_ARGS__)

// Select current data as data.
// For jarr, jtuple, it's the nth element of current data.
constexpr auto jcur = [](auto& d) -> auto& { return d; };
constexpr auto jauto = jcur;


template<strlit Name>
struct jref_t
{
    constexpr decltype(auto) operator()(auto&& p, auto&& j, auto&& d, auto&&... arg) const
    requires(
        requires{ sub_chain(p)[idx_c<0>](remove_first_n_elms<1>(sub_chain(p)), j, d, arg...); })
    {
        auto r = sub_chain(p);
        return r[idx_c<0>](remove_first_n_elms<1>(r), j, d, arg...);
    };

    // child ->...-> Name ->... -> parent
    constexpr auto sub_chain(_tuple_ auto&& p) const
    {
        //auto pa = group_elms<
        //    ct_chained_invoke(
        //        DSK_NO_CVREF_TYPE_LIST(p),
        //        ct_start_at_first(DSK_TYPE_EXP(T::name == Name)),
        //        ct_split_at_first_n(1))
        //>(p);

        static_assert(p.count);
        return select_elms_start_at_first(p, DSK_TYPE_EXP(std::remove_cvref_t<T>::name == Name));
    }
};

template<strlit Name>
inline constexpr auto jref = jref_t<Name>();



template<strlit Name, _tuple_ KVs>
struct jobj_t
{
    static_assert(KVs::count >= 2);
    static_assert(KVs::count % 2 == 0);

    using is_jobj = void;

    constexpr static auto name = Name;
    constexpr static auto key_type_list = ct_flatten(select_even_types<KVs>(), []<class K, auto I>()
    {
        if constexpr(_jarg_nested_<K>) return elm_t<I*2 + 1, KVs>::key_type_list;
        else                           return type_list<K>;
    });

    using _check_key_type_list = decltype([]()
    {
        if constexpr(ct_all_of(key_type_list, DSK_TYPE_EXP(_jkey_<T>)))
        {
            static_assert(ct_is_unique(key_type_list, DSK_TYPE_EXP(T::i)), "duplicated key");
        }
        else
        {
            static_assert(ct_all_of(key_type_list, DSK_TYPE_EXP(_char_str_<T>)), "unsupported key type");
        }
    }());


    DSK_NO_UNIQUE_ADDR KVs kvs;

    constexpr decltype(auto) chain(_tuple_ auto&& p) const
    {
        if constexpr(name.size()) return tuple_cat(fwd_as_tuple(*this), p);
        else                      return DSK_FORWARD(p);
    }

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, auto const& d) const
    {
        j.obj_scope(kvs.count/2, [&]()
        {
            return write_jflds(chain(p), j, d, kvs);
        });
    }

    constexpr size_t operator()(auto&& p, _jwriter_ auto&& j, auto const& d, jarg_nested) const
    {
        return write_jflds(chain(p), j, d, kvs);
    }

    constexpr auto operator()(auto&& p, auto&& j, auto&& d) const
    {
        return read_jobj(chain(p), j, d, kvs);
    }

    constexpr auto operator()(auto&& p, auto&& j, auto&& d, jarg_nested) const
    {
        return read_jobj(chain(p), j, d, kvs);
    }
};


template<strlit Name = "">
constexpr auto jobj(auto&&... kvs)
{
    return jobj_t<
        Name,
        typename decltype(
            ct_transform<tuple<decltype(kvs)...>>(DSK_TYPE_IDX_EXP(
                         std::conditional<I % 2 == 0 && _char_str_<T>, std::string_view, std::decay_t<T>>()))
        )::type
    >{
        {DSK_FORWARD(kvs)...}
    };
}


// By default, associative containers are just treated as range.
// Use this to force it to treat them as an jobj.
// Keys can be string and numbers, and typically won't be escaped by writer/reader.
// v: represents mapped_type
template<class... V>
struct jmap_obj
{
    static_assert(sizeof...(V) <= 1);

    using is_jobj = void;

    DSK_NO_UNIQUE_ADDR tuple<V...> v;

    constexpr explicit jmap_obj(auto&&... v_)
        : v(DSK_FORWARD(v_)...)
    {}

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, auto const& d) const
    {
        j.obj_scope(d.size(), [&]()
        {
            for(auto&[key, mapped] : d)
            {
                j.fld_scope(key, [&]()
                {
                    apply_elms(v, [&](auto&... v)
                    {
                        write_jval(p, j, mapped, v...);
                    });
                });
            }

            return d.size();
        });
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&& d) const
    {
        DSK_U_TRY_FWD(o, get_jobj(j));

        using key_t = typename DSK_NO_CVREF_T(d)::key_type;

        return foreach_jfld<key_t>(o, [&](auto&& key, auto&& fv) -> error_code
        {
            typename DSK_NO_CVREF_T(d)::mapped_type mapped;

            DSK_E_TRY_ONLY(apply_elms(v, [&](auto&... v)
            {
                return read_jval(p, fv, mapped, v...);
            }));

            if(d.try_emplace(DSK_FORWARD(key), mut_move(mapped)).second)
            {
                return {};
            }

            return jerrc::duplicated_key;
        });
    }
};

template<class... V>
jmap_obj(V...) -> jmap_obj<V...>;


// Maps to range of v.
// If no v is specified, first M elements of range_vaule_t<d> is used.
// If no v or M is specified, whole d is used.
// If Exact is true, each sub array must be of same size as specified by M and v when read. 
// NOTE: empty range is always write out, while read can handle missing field by clear_range.
template<size_t M = 0, bool Exact = false>
constexpr auto jarr_of(auto&&... v)
{
    return [...v = DSK_FORWARD(v)](auto&& p, auto&& j, std::ranges::range auto&& d)
    {
        if constexpr(_jwriter_<decltype(j)>)
        {
            write_jarr_of<M>(p, j, d, v...);
        }
        else
        {
            return read_jarr_of<M, Exact>(p, j, d, v...);
        }
    };
}

template<size_t M = 0>
constexpr auto jarr_of_exact(auto&&... v)
{
    return jarr_of<M, true>(DSK_FORWARD(v)...);
}

// Similar to jarr_of<M>(...); But if implementation supports
// packed array, it should pack them as array of same type T.
// If T is different from actual element type, static_cast<T>(elm) is used.
// Only when no v is specified and the whole d is used, the
// implementation may memcpy from/to d.
// NOTE: empty range is always write out, while read can handle missing field by clear_range.
template<class T, size_t M = 0, bool Exact = false>
constexpr auto jpacked_arr_of(auto&&... v)
{
    return [...v = DSK_FORWARD(v)](auto&& p, auto&& j, std::ranges::range auto&& d)
    {
        if constexpr(_jwriter_<decltype(j)>)
        {
            write_jpacked_arr_of<T, M>(p, j, d, v...);
        }
        else
        {
            return read_jpacked_arr_of<T, M, Exact>(p, j, d, v...);
        }
    };
}

template<size_t M = 0>
constexpr auto jpacked_arr_of_exact(auto&&... v)
{
    return jpacked_arr_of<M, true>(DSK_FORWARD(v)...);
}

// Maps vs to arr.
// If both vs and M are specified, sizeof...(vs) == M.
// If no v is specified, d should be _tuple_like_, and first M elements of d is used.
// If no v or M is specified, whole d is used.
// If Exact is true, array must be of same size as specified by M and v when read. 
template<size_t M = 0, bool Exact = false>
constexpr auto jarr(auto&&... vs)
{
    return [...vs = DSK_FORWARD(vs)](auto&& p, auto&& j, auto&& d)
    {
        if constexpr(_jwriter_<decltype(j)>)
        {
            write_jarr<M>(p, j, d, vs...);
        }
        else
        {
            return read_jarr<M, Exact>(p, j, d, vs...);
        }
    };
}

template<size_t M = 0>
constexpr auto jarr_exact(auto&&... vs)
{
    return jarr<M, true>(DSK_FORWARD(vs)...);
}

// Each v is map to element of _tuple_like_ d *in order*.
// So, you don't need to jsel for each v like in jarr().
// If both vs and M are specified, sizeof...(vs) == M.
// If no v is specified, first M elements of d is used.
// If no v or M is specified, whole d is used.
// If Exact is true, array must be of same size as specified by M and v when read. 
template<size_t M = 0, bool Exact = false>
constexpr auto jtuple(auto&&... vs)
{
    return [...vs = DSK_FORWARD(vs)](auto&& p, auto&& j, _tuple_like_ auto&& d)
    {
        if constexpr(_jwriter_<decltype(j)>)
        {
            write_jtuple<M>(p, j, d, vs...);
        }
        else
        {
            return read_jtuple<M, Exact>(p, j, d, vs...);
        }
    };
}

template<size_t M = 0>
constexpr auto jtuple_exact(auto&&... vs)
{
    return jtuple<M, true>(DSK_FORWARD(vs)...);
}


constexpr auto jbinary = [](auto&& /*p*/, auto&& j, auto&& d)
{
    if constexpr(_jwriter_<decltype(j)>)
    {
        j.binary(d);
    }
    else
    {
        return get_jbinary(j, d);
    }
};


template<class V>
struct joptional
{
    DSK_NO_UNIQUE_ADDR V v;

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, _optional_ auto const& d) const
    {
        if(d) write_jval(p, j, *d, v);
        else  j.null();
    }

    template<class K>
    constexpr void operator()(auto&& p, _jwriter_ auto&& j, _optional_ auto const& d, jarg_key<K> arg) const
    {
        if(d)
        {
            j.fld_scope(arg.key, [&]()
            {
                write_jval(p, j, *d, v);
            });
        }
    }

    constexpr error_code operator()(auto&& p, auto&& j, _optional_ auto& d) const
    {
        DSK_U_TRY_FWD(null, check_jnull(j));

        if(! null)
        {
            return read_jval(p, j, assure_val(d), v);
        }

        d.reset();
        return {};
    }

    constexpr void operator()(auto&& /*p*/, jarg_no_fld, _optional_ auto& d, jarg_no_fld) const
    {
        d.reset();
    }
};

template<class V>
joptional(V) -> joptional<V>;


enum jnullable_e
{
    jnullable = true,
    jnon_nullable = false
};

template<jnullable_e Nullable, class GetAlt, class... Flds>
struct jvariant_t
{
    constexpr static auto key_type_list = DSK_CONDITIONAL_V_NO_CAPTURE(
                                            _null_op_<GetAlt>, (... + Flds::key_type_list),
                                                               type_list<>);

    DSK_NO_UNIQUE_ADDR GetAlt         getAlt;
    DSK_NO_UNIQUE_ADDR tuple<Flds...> flds;

    constexpr explicit jvariant_t(auto&& getAlt_, auto&&... flds_)
        : getAlt(DSK_FORWARD(getAlt_)), flds(DSK_FORWARD(flds_)...)
    {}

    template<_variant_ T>
    constexpr void check_var(T const&) const
    {
        constexpr auto varSize = std::variant_size_v<T>;
        static_assert(varSize >= 2);
        static_assert(1 + flds.count == varSize);
        static_assert(_same_as_<std::monostate, std::variant_alternative_t<0, T>>);
    }

    constexpr auto write_var(auto& p, auto& j, auto& d, auto... nested) const
    {
        check_var(d);

        DSK_ASSERT(d.index());

        return visit_var(d, [&]<size_t I>(auto& fd)
        {
            if constexpr(I)
            {
                if constexpr(sizeof...(nested)) return flds[idx_c<I - 1>](p, j, fd, nested...);
                else                            write_jval(p, j, fd, flds[idx_c<I - 1>]);
            }
            else if constexpr(sizeof...(nested))
            {
                return size_t();
            }
        });
    }

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, _variant_ auto const& d) const
    {
        if constexpr(Nullable)
        {
            if(d.index() == 0)
            {
                j.null();
                return;
            }
        }

        write_var(p, j, d);
    }

    constexpr size_t operator()(auto&& p, _jwriter_ auto&& j, _variant_ auto const& d, jarg_nested) const
    requires(
        ! Nullable)
    {
        return write_var(p, j, d, jarg_nested());
    }

    template<class K>
    constexpr void operator()(auto&& p, _jwriter_ auto&& j, _variant_ auto const& d, jarg_key<K> arg) const
    {
        if constexpr(Nullable)
        {
            if(d.index() == 0)
                return;
        }

        j.fld_scope(arg.key, [&]()
        {
            write_var(p, j, d);
        });
    }

    constexpr error_code read_var(auto& p, auto& j, auto& d, auto... nested) const
    {
        check_var(d);

        auto readFd = [&]<size_t I>(auto& fd) -> error_code
        {
            if constexpr(I)
            {
                if constexpr(sizeof...(nested)) return flds[idx_c<I - 1>](p, j, fd, nested...);
                else                            return read_jval(p, j, fd, flds[idx_c<I - 1>]);
            }
            else
            {
                if constexpr(Nullable) return {};
                else                   return jerrc::non_nullable_field;
            }
        };

        if constexpr(! _null_op_<GetAlt>)
        {
            DSK_U_TRY_FWD(i, getAlt(j, d));

            return assure_var_holds(d, i, readFd);
        }
        else
        {
            return visit_var(d, readFd);
        }
    }

    constexpr error_code operator()(auto&& p, auto&& j, _variant_ auto& d) const
    requires(
        ! _null_op_<GetAlt>)
    {
        return read_var(p, j, d);
    }

    constexpr error_code operator()(auto&& p, auto&& j, _variant_ auto& d, jarg_nested) const
    requires(
        ! Nullable && _null_op_<GetAlt>)
    {
        // j and d should already be non null, as this is nested.
        DSK_ASSERT(d.index());
        return read_var(p, j, d, jarg_nested());
    }

    constexpr void operator()(auto&& /*p*/, jarg_no_fld, _variant_ auto& d, jarg_no_fld) const
    requires(
        (bool)Nullable)
    {
        check_var(d);
        d.template emplace<0>();
    }
};

// the first field of variant should be std::monostate, so flds start from second field.
// getAlt: for read. getAlt(jval, d) -> _errorable_/_integer_; return the expected alternative index.
template<jnullable_e Nullable = jnullable>
constexpr auto jvariant_s(auto&& getAlt, auto&&... flds)
{
    return make_templated_nt1<jvariant_t, Nullable>(DSK_FORWARD(getAlt), DSK_FORWARD(flds)...);
}

// use with jnested key, so each of its jobj flds are as-if merged into parent jobj.
constexpr auto jnested_variant(_jobj_ auto&&... flds)
{
    return jvariant_s<jnon_nullable>(null_op, DSK_FORWARD(flds)...);
}

// variant's active type only depends on jval's type,
// so similar type can only present once in variant's type list.
// obj/arr can also be used.
template<jnullable_e Nullable = jnullable>
constexpr auto jval_variant(auto&&... flds)
{
    return jvariant_s<Nullable>(
        [](auto& jv, _variant_ auto& d)
        {
            constexpr auto types = ct_transform(type_list<std::monostate, decltype(flds)...>, []<class T, auto I>()
            {
                if constexpr(_jobj_<T>) return type_c<T>;
                else                    return std::variant_alternative<I, DSK_NO_CVREF_T(d)>();
            });

            return get_jval_matched_type(jv, types);
        },
        DSK_FORWARD(flds)...
    );
}


template<class T>
using _j_assignable_t = typename decltype([]()
{
    using ncvr_t = std::remove_cvref_t<T>;

    if constexpr(requires{ requires _str_<ncvr_t> && ! std::is_assignable_v<ncvr_t&, std::string_view>; })
    {
        return type_c<std::string_view>;
    }
    else
    {
        return type_c<ncvr_t>;
    }
}())::type;


// when write: write(to(d));
// when read: _j_assignable_t<decltype(to(d))> td; read(td); from(td, d);
template<class To, class From, class... V>
struct jconv
{
    static_assert(sizeof...(V) <= 1);

    DSK_NO_UNIQUE_ADDR To          to;
    DSK_NO_UNIQUE_ADDR From        from;
    DSK_NO_UNIQUE_ADDR tuple<V...> v;

    constexpr jconv(auto&& to_, auto&& from_, auto&&... v_)
        : to(DSK_FORWARD(to_)), from(DSK_FORWARD(from_)), v(DSK_FORWARD(v_)...)
    {}

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, auto const& d) const
    {
        apply_elms(v, [&](auto&... v)
        {
            write_jval(p, j, to(d), v...);
        });
    }

    constexpr decltype(auto) operator()(auto&& p, _jwriter_ auto&& j, auto const& d, auto&& arg) const
    requires(
        requires{ v[idx_c<0>](p, j, to(d), arg); })
    {
        return v[idx_c<0>](p, j, to(d), arg);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&& d) const
    {
        _j_assignable_t<decltype(to(d))> td;

        DSK_E_TRY_ONLY(apply_elms(v, [&](auto&... v)
        {
            return read_jval(p, j, td, v...);
        }));

        return do_from(td, d);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&& d, auto&& arg) const
    requires(
        requires(_j_assignable_t<decltype(to(d))> td){ v[idx_c<0>](p, j, td, arg); })
    {
        _j_assignable_t<decltype(to(d))> td;

        DSK_E_TRY_ONLY(v[idx_c<0>](p, j, td, arg));

        return do_from(td, d);
    }

    constexpr auto do_from(auto&& td, auto& d) const
    {
        if constexpr(requires{ {from(td, d)} -> _void_; })
        {
            from(td, d);
            return jerrc();
        }
        else if constexpr(requires{ {from(td, d)} -> _same_as_<bool>; })
        {
            if(from(td, d)) return jerrc();
            else            return jerrc::user_conv_failed;
        }
        else
        {
            static_assert(_errorable_<decltype(from(td, d))>);
            return from(td, d);
        }
    }
};

template<class To, class From, class... V>
jconv(To, From, V...) -> jconv<To, From, V...>;


// when write, assert(check(d)) before write out;
// when  read, check(d) after read;
// check(d) may return bool or _errorable_;
template<class Check, class... V>
struct jcheck
{
    static_assert(sizeof...(V) <= 1);

    DSK_NO_UNIQUE_ADDR Check       check;
    DSK_NO_UNIQUE_ADDR tuple<V...> v;

    constexpr jcheck(auto&& check_, auto&&... v_)
        : check(DSK_FORWARD(check_)), v(DSK_FORWARD(v_)...)
    {}

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, auto const& d) const
    {
        DSK_ASSERT(! has_err(do_check(d)));

        apply_elms(v, [&](auto&... v)
        {
            write_jval(p, j, d, v...);
        });
    }

    constexpr decltype(auto) operator()(auto&& p, _jwriter_ auto&& j, auto const& d, auto&& arg) const
    requires(
        requires{ v[idx_c<0>](p, j, d, arg); })
    {
        DSK_ASSERT(! has_err(do_check(d)));

        return v[idx_c<0>](p, j, d, arg);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&& d) const
    {
        DSK_E_TRY_ONLY(apply_elms(v, [&](auto&... v)
        {
            return read_jval(p, j, d, v...);
        }));

        return do_check(d);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&& d, auto&& arg) const
    requires(
        requires{ v[idx_c<0>](p, j, d, arg); })
    {
        DSK_E_TRY_ONLY(v[idx_c<0>](p, j, d, arg));
        return do_check(d);
    }

    constexpr auto do_check(auto& d) const
    {
        if constexpr(_same_as_<decltype(check(d)), bool>)
        {
            if(check(d)) return jerrc();
            else         return jerrc::user_check_failed;
        }
        else
        {
            static_assert(_errorable_<decltype(check(d))>);
            return check(d);
        }
    }

    // for check whole jobj parsed results

    constexpr static auto key_type_list = type_list<>;

    constexpr size_t operator()(auto&&, _jwriter_ auto&&, [[maybe_unused]] auto const& d, jarg_nested) const
    {
        static_assert(sizeof...(V) == 0);
        DSK_ASSERT(! has_err(do_check(d)));
        return 0;
    }

    constexpr error_code operator()(auto&&, auto&&, auto&& d, jarg_nested) const
    {
        static_assert(sizeof...(V) == 0);
        return do_check(d);
    }
};

template<class Check, class... V>
jcheck(Check, V...) -> jcheck<Check, V...>;

constexpr auto jcheck_eq_to(auto&& u, auto&&... v)
{
    return jcheck([u = DSK_FORWARD(u)](auto& d){ return u == d; }, DSK_FORWARD(v)...);
}


// when write, write out the default data(def or def());
// when  read,  read into a temporary data, then exam it with exam(d);
// exam(d) may return bool or _errorable_;
template<class Default, class Exam, class... V>
struct jexam
{
    static_assert(sizeof...(V) <= 1);

    DSK_NO_UNIQUE_ADDR Default     def;
    DSK_NO_UNIQUE_ADDR Exam        exam;
    DSK_NO_UNIQUE_ADDR tuple<V...> v;

    constexpr jexam(auto&& def_, auto&& exam_, auto&&... v_)
        : def(DSK_FORWARD(def_)), exam(DSK_FORWARD(exam_)), v(DSK_FORWARD(v_)...)
    {}

    constexpr decltype(auto) get_default() const
    {
        if constexpr(requires{ def(); }) return def();
        else                             return (def);
    }

    constexpr void operator()(auto&& p, _jwriter_ auto&& j, auto const&) const
    {
        apply_elms(v, [&](auto&... v)
        {
            write_jval(p, j, get_default(), v...);
        });
    }

    constexpr decltype(auto) operator()(auto&& p, _jwriter_ auto&& j, auto const&, auto&& arg) const
    requires(
        requires{ v[idx_c<0>](p, j, get_default(), arg); })
    {
        return v[idx_c<0>](p, j, get_default(), arg);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&&) const
    {
        _j_assignable_t<decltype(get_default())> d;

        DSK_E_TRY_ONLY(apply_elms(v, [&](auto&... v)
        {
            return read_jval(p, j, d, v...);
        }));

        return do_exam(d);
    }

    constexpr error_code operator()(auto&& p, auto&& j, auto&&, auto&& arg) const
    requires(
        requires(_j_assignable_t<decltype(get_default())> d){ v[idx_c<0>](p, j, d, arg); })
    {
        _j_assignable_t<decltype(get_default())> d;

        DSK_E_TRY_ONLY(v[idx_c<0>](p, j, d, arg));

        return do_exam(d);
    }

    constexpr auto do_exam(auto const& d) const
    {
        if constexpr(_null_op_<decltype(exam)>)
        {
            if(d == get_default()) return jerrc();
            else                   return jerrc::user_exam_failed;
        }
        else
        {
            if constexpr(requires{ {exam(d)} -> _same_as_<bool>; })
            {
                if(exam(d)) return jerrc();
                else        return jerrc::user_exam_failed;
            }
            else
            {
                static_assert(_errorable_<decltype(exam(d))>);
                return exam(d);
            }
        }
    }
};

template<class Default, class Exam, class... V>
jexam(Default, Exam, V...) -> jexam<Default, Exam, V...>;


constexpr auto jexam_eq_to(auto&& default_, auto&&... v)
{
    return jexam(DSK_FORWARD(default_), null_op, DSK_FORWARD(v)...);
}


template<bool Escape>
struct jescape_t
{
    constexpr void operator()(auto&& /*p*/, _jwriter_ auto&& j, auto const& d) const
    {
        if constexpr(Escape) j.str(d);
        else                 j.str_noescape(d);
    };

    constexpr auto operator()(auto&& /*p*/, auto&& j, auto& d) const
    {
        if constexpr(Escape) return get_jstr(j, d);
        else                 return get_jstr_noescape(j, d);
    };
};

constexpr auto   jescape = jescape_t< true>();
constexpr auto jnoescape = jescape_t<false>();


// For read only. If a key exists, assign d true, otherwise false;
inline constexpr struct jexist_t
{
    constexpr void operator()(auto&& /*p*/, auto&& j, auto& d) const noexcept
    {
        static_assert(! _jwriter_<decltype(j)>);
        d = true;
    }

    constexpr void operator()(auto&& /*p*/, jarg_no_fld, auto& d, jarg_no_fld) const noexcept
    {
        d = false;
    }
} jexist;


inline constexpr struct jcheck_exist_t
{
    constexpr void operator()(auto&& /*p*/, auto&& j, auto& /*d*/) const noexcept
    {
        static_assert(! _jwriter_<decltype(j)>);
    }

    constexpr jerrc operator()(auto&& /*p*/, jarg_no_fld, auto& /*d*/, jarg_no_fld) const noexcept
    {
        return jerrc::field_not_found;
    }
} jcheck_exist;


} // namespace dsk
