#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <variant>


namespace dsk{


template<class T> concept _jwriter_ = requires{ typename std::remove_cvref_t<T>::is_jwriter; };
template<class T> concept _jreader_ = requires{ typename std::remove_cvref_t<T>::is_jreader; };


constexpr void jwrite(auto&& def, auto const& v, _jwriter_ auto&& w)
{
    def(tuple(), w, v);
}

constexpr void jwrite_range(auto&& def, std::ranges::range auto const& vs, _jwriter_ auto&& w)
{
    for(auto& v : vs)
    {
        def(tuple(), w, v);
        
        if constexpr(requires{ w.write_delimiter(); })
        {
            w.write_delimiter();
        }
    }
}


// returns offset after last read char.
constexpr expected<size_t> jread(auto&& def, _jreader_ auto&& reader, auto&& input, auto& v,
                                 size_t offset = 0, auto&&... opts)
{
    return reader.read
    (
        DSK_FORWARD(input),
        [&](auto& jv)
        {
            return def(tuple(), jv, v);
        },
        offset,
        DSK_FORWARD(opts)...
    );
}


enum juse_general_foreach_e
{
    juse_general_foreach = true,
    jdont_use_general_foreach = false,
};

// f can return false for early return.
// returns offset after last read char.
template<
    class T,
    juse_general_foreach_e UseGeneral = jdont_use_general_foreach
>
constexpr expected<size_t> jread_foreach(auto&& def, _jreader_ auto&& reader, auto&& input, auto&& f,
                                         size_t offset = 0, auto&&... opts)
{
    auto rf = [&](T&& v) -> expected<bool>
    {
        using fr_type = decltype(f(mut_move(v)));

        if constexpr(_void_<fr_type>)
        {
            f(mut_move(v));
            return true;
        }
        else if constexpr(_same_as_<fr_type, bool>)
        {
            return f(mut_move(v));
        }
        else if constexpr(_result_of_<fr_type, bool>)
        {
            DSK_E_TRY_RETURN(f(mut_move(v)));
        }
        else
        {
            DSK_E_TRY_ONLY(f(mut_move(v)));
            return true;
        }
    };

    if constexpr(! UseGeneral
                 && requires{ reader.read_foreach(DSK_FORWARD(input), [](auto&&){ return expected<bool>(true); },
                                                  offset, DSK_FORWARD(opts)...); })
    {
        return reader.read_foreach
        (
            DSK_FORWARD(input),
            [&](auto& jv) -> expected<bool>
            {
                T v;
                DSK_E_TRY_ONLY(def(tuple(), jv, v));
                return rf(mut_move(v));
            },
            offset,
            DSK_FORWARD(opts)...
        );
    }
    else
    {
        for(size_t len = buf_size(input);;)
        {
            T v;

            auto rof = jread(def, reader, input, v, offset, opts...);
            
            if(has_err(rof))
            {
                break;
            }

            DSK_E_TRY(bool go_on, rf(mut_move(v)));

            offset = get_val(rof);

            if constexpr(requires{ reader.find_doc(input, offset); })
            {
                size_t pos = reader.find_doc(input, offset);

                if(pos == npos)
                {
                    break;
                }

                offset = pos;
            }

            if(! go_on || offset >= len)
            {
                break;
            }
        }

        return offset;
    }
}

template<juse_general_foreach_e UseGeneral = jdont_use_general_foreach>
constexpr expected<size_t> jread_range(auto&& def, _jreader_ auto&& reader, auto&& input, auto& vs,
                                       size_t offset = 0, auto&&... opts)
{
    return jread_foreach<typename DSK_NO_CVREF_T(vs)::value_type, UseGeneral>
    (
        DSK_FORWARD(def),
        DSK_FORWARD(reader),
        DSK_FORWARD(input),
        [&](auto&& v)
        {
            vs.emplace_back(mut_move(v));
        },
        offset,
        DSK_FORWARD(opts)...
    );
}

template<juse_general_foreach_e UseGeneral = jdont_use_general_foreach>
constexpr expected<size_t> jread_range_at_most(auto&& def, _jreader_ auto&& reader, auto&& input, size_t maxV, auto& vs,
                                               size_t offset = 0, auto&&... opts)
{
    if(maxV <= 0)
    {
        return 0;
    }

    return jread_foreach<DSK_NO_CVREF_T(vs)::value_type, UseGeneral>
    (
        DSK_FORWARD(def),
        DSK_FORWARD(reader),
        DSK_FORWARD(input),
        [&](auto&& v)
        {
            vs.emplace_back(mut_move(v));
            return bool(--maxV);
        },
        offset,
        DSK_FORWARD(opts)...
    );
}

constexpr error_code jread_whole(auto&& def, _jreader_ auto&& reader, auto&& input, auto& v,
                                 size_t offset = 0, auto&&... opts)
{
    DSK_E_TRY(size_t of, jread(DSK_FORWARD(def), DSK_FORWARD(reader), DSK_FORWARD(input), v, offset, DSK_FORWARD(opts)...));

    if(of != buf_size(input)) return errc::input_not_fully_consumed;
    else                      return {};
}

template<juse_general_foreach_e UseGeneral = jdont_use_general_foreach>
constexpr error_code jread_range_whole(auto&& def, _jreader_ auto&& reader, auto&& input, auto& vs,
                                       size_t offset = 0, auto&&... opts)
{
    DSK_E_TRY(size_t of, jread_range<UseGeneral>(DSK_FORWARD(def), DSK_FORWARD(reader), DSK_FORWARD(input), vs, offset, DSK_FORWARD(opts)...));

    if(of != buf_size(input)) return errc::input_not_fully_consumed;
    else                      return {};
}


struct jarg_no_fld{};
template<class T> concept _jarg_no_fld_ = _no_cvref_same_as_<T, jarg_no_fld>;

inline constexpr struct jarg_nested{} jnested;
template<class T> concept _jarg_nested_ = _no_cvref_same_as_<T, jarg_nested>;

template<class T>
struct jarg_key
{
    T const& key;
};

template<class T>
jarg_key(T) -> jarg_key<T>;

template<class T> void _jarg_key_impl(jarg_key<T>&);
template<class T> concept _jarg_key_ = requires(std::remove_cvref_t<T>& t){ dsk::_jarg_key_impl(t); };


using jkey_uint_t = uint32_t;

template<jkey_uint_t I>
struct jkey
{
    using uint_t = decltype([]()
    {
             if constexpr(I <=  uint8_t(-1)) return  uint8_t();
        else if constexpr(I <= uint16_t(-1)) return uint16_t();
        else if constexpr(I <= uint32_t(-1)) return uint32_t();
        else                                 return uint64_t();
    }());

    static constexpr uint_t i = I;
    std::string_view s;
};


template<jkey_uint_t I> void _jkey_impl(jkey<I>&);
template<class T> concept _jkey_ = requires(std::remove_cvref_t<T>& t){ dsk::_jkey_impl(t); };


constexpr auto& jkey_str(auto& k) noexcept
{
    if constexpr(_jkey_<decltype(k)>)
    {
        return k.s;
    }
    else
    {
        static_assert(_same_as_<decltype(k), std::string_view const&>);
        return k;
    }
}

constexpr decltype(auto) jkey_uint_or_str(auto& k) noexcept
{
    if constexpr(_jkey_<decltype(k)>)
    {
        return k.i;
    }
    else
    {
        static_assert(_same_as_<decltype(k), std::string_view const&>);
        return k;
    }
}


constexpr auto& jkey_str_or_as_is(auto& k) noexcept
{
    if constexpr(_jkey_<decltype(k)>) return k.s;
    else                              return k;
}

constexpr decltype(auto) jkey_uint_or_as_is(auto& k) noexcept
{
    if constexpr(_jkey_<decltype(k)>) return k.i;
    else                              return k;
}


template<class T> concept _jobj_ = requires{ typename std::remove_cvref_t<T>::is_jobj; };



// the value is consumed only if it's null.
inline constexpr struct check_jnull_cpo
{
    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires
        requires{ dsk_check_jnull(*this, v); }
    {
        return dsk_check_jnull(*this, v);
    }

    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires(
        requires{ v.check_null(); } && ! requires{ dsk_check_jnull(*this, v); })
    {
        return v.check_null();
    }
} check_jnull;


inline constexpr struct get_jstr_cpo
{
    constexpr decltype(auto) operator()(auto& v, auto& t) const noexcept
    requires
        requires{ dsk_get_jstr(*this, v, t); }
    {
        return dsk_get_jstr(*this, v, t);
    }

    constexpr decltype(auto) operator()(auto& v, auto& t) const noexcept
    requires(
        requires{ v.get_str(t); } && ! requires{ dsk_get_jstr(*this, v, t); })
    {
        return v.get_str(t);
    }
} get_jstr;


inline constexpr struct get_jstr_noescape_cpo
{
    constexpr decltype(auto) operator()(auto& v, auto& t) const noexcept
    requires
        requires{ dsk_get_jstr_noescape(*this, v, t); }
    {
        return dsk_get_jstr_noescape(*this, v, t);
    }
} get_jstr_noescape;


inline constexpr struct get_jbinary_cpo
{
    constexpr decltype(auto) operator()(auto& v, auto& t) const noexcept
    requires
        requires{ dsk_get_jbinary(*this, v, t); }
    {
        return dsk_get_jbinary(*this, v, t);
    }
} get_jbinary;


inline constexpr struct get_jarithmetic_cpo
{
    constexpr decltype(auto) operator()(auto& v, _arithmetic_ auto& t) const noexcept
    requires
        requires{ dsk_get_jarithmetic(*this, v, t); }
    {
        return dsk_get_jarithmetic(*this, v, t);
    }

    constexpr decltype(auto) operator()(auto& v, _arithmetic_ auto& t) const noexcept
    requires(
        requires{ v.get_arithmetic(t); } && ! requires{ dsk_get_jarithmetic(*this, v, t); })
    {
        return v.get_arithmetic(t);
    }
} get_jarithmetic;


inline constexpr struct get_jobj_cpo
{
    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires
        requires{ dsk_get_jobj(*this, v); }
    {
        return dsk_get_jobj(*this, v);
    }
} get_jobj;


inline constexpr struct get_jarr_iter_cpo
{
    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires
        requires{ dsk_get_jarr_iter(*this, v); }
    {
        return dsk_get_jarr_iter(*this, v);
    }

    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires(
        requires{ v.arr_iter(); } && ! requires{ dsk_get_jarr_iter(*this, v); })
    {
        return v.arr_iter();
    }
} get_jarr_iter;


template<class Key>
struct foreach_jfld_cpo
{
    constexpr decltype(auto) operator()(auto& o, auto&& h) const noexcept
    requires
        requires{ dsk_foreach_jfld(*this, o, DSK_FORWARD(h)); }
    {
        return dsk_foreach_jfld(*this, o, DSK_FORWARD(h));
    }
};
template<class Key>
inline constexpr foreach_jfld_cpo<Key> foreach_jfld;


inline constexpr struct get_jval_finder_cpo
{
    constexpr decltype(auto) operator()(auto& o) const noexcept
    requires
        requires{ dsk_get_jval_finder(*this, o); }
    {
        return dsk_get_jval_finder(*this, o);
    }
} get_jval_finder;


inline constexpr struct foreach_found_jval_cpo
{
    constexpr decltype(auto) operator()(auto& finder, auto& mks, auto&& h) const noexcept
    requires
        requires{ dsk_foreach_found_jval(*this, finder, mks, DSK_FORWARD(h)); }
    {
        return dsk_foreach_found_jval(*this, finder, mks, DSK_FORWARD(h));
    }
} foreach_found_jval;


inline constexpr struct get_jval_matched_type_cpo
{
    constexpr decltype(auto) operator()(auto& v, auto const& l) const noexcept
    requires
        requires{ dsk_get_jval_matched_type(*this, v, l); }
    {
        return dsk_get_jval_matched_type(*this, v, l);
    }
} get_jval_matched_type;


template<class T>
struct get_jpacked_arr_cpo
{
    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires
        requires{ dsk_get_jpacked_arr(*this, v); }
    {
        return dsk_get_jpacked_arr(*this, v);
    }

    constexpr decltype(auto) operator()(auto& v) const noexcept
    requires(
        requires{ v.template packed_arr<T>(); } && ! requires{ dsk_get_jpacked_arr(*this, v); })
    {
        return v.template packed_arr<T>();
    }
};

template<class T>
inline constexpr get_jpacked_arr_cpo<T> get_jpacked_arr;



auto& jaccess_val(auto& d, auto& v) noexcept
{
         if constexpr(requires{     v(d); }) return     v(d);
    else if constexpr(requires{  d.*v   ; }) return  d.*v   ;
    else if constexpr(requires{ (d.*v)(); }) return (d.*v)();
    else                                     return     v   ;
}

auto& jaccess_val(auto& d) noexcept
{
    return d;
}


} // namespace dsk
