#pragma once

#include <dsk/optional.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/endian.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/num_cast.hpp>
#include <dsk/util/from_str.hpp>


namespace dsk{


// ref:
// https://github.com/postgres/postgres/tree/master/src/backend/utils/adt
// https://github.com/pshampanier/libpqmxx/blob/master/src/postgres-oids.cpp
// https://github.com/pferdinand/libpqmxx/blob/master/src/postgres-params.cpp#L226
// https://stackoverflow.com/questions/4016412/postgresqls-libpq-encoding-for-binary-transport-of-array-data
// https://www.npgsql.org/dev/oids.html


enum pq_fmt_e
{
    pq_fmt_text   = 0,
    pq_fmt_binary = 1
};


// https://github.com/postgres/postgres/blob/master/src/include/catalog/pg_type.dat
// postgres install folder/include/server/catalog/pg_type_d.h
enum pq_oid_e : Oid
{
    pq_oid_invalid = 0,

    pq_oid_unknown = 705,

    pq_oid_bool    = 16,
    pq_oid_int2    = 21,
    pq_oid_int4    = 23,
    pq_oid_int8    = 20,
    pq_oid_float4  = 700,
    pq_oid_float8  = 701,
    pq_oid_numeric = 1700, // r/w binary format unsupported yet
    
    pq_oid_char    = 18,
    pq_oid_bpchar  = 1042, // fixed length, blank padded string
    pq_oid_varchar = 1043,
    pq_oid_name    = 19,   // 63 bytes + 1 byte terminator
    pq_oid_text    = 25,
    
    pq_oid_json    = 114,
    pq_oid_jsonb   = 3802,
    pq_oid_xml     = 142,
    
    pq_oid_bytea   = 17,

    // array

    pq_oid_arr_bool    = 1000,
    pq_oid_arr_int2    = 1005,
    pq_oid_arr_int4    = 1007,
    pq_oid_arr_int8    = 1016,
    pq_oid_arr_float4  = 1021,
    pq_oid_arr_float8  = 1022,
    pq_oid_arr_numeric = 1231,

    pq_oid_arr_char    = 1002,
    pq_oid_arr_bpchar  = 1014,
    pq_oid_arr_varchar = 1015,
    pq_oid_arr_text    = 1009,

    pq_oid_arr_json    = 199,
    pq_oid_arr_jsonb   = 3807,

    pq_oid_arr_bytea   = 1001,
};


constexpr bool is_textual(pq_oid_e t) noexcept
{
    switch(t)
    {
        case pq_oid_char   : case pq_oid_name: case pq_oid_text  :
        case pq_oid_json   : case pq_oid_xml : case pq_oid_bpchar:
        case pq_oid_varchar:
            return true;
        default:
            return false;
    }
}

constexpr bool is_array(pq_oid_e t) noexcept
{
    switch(t)
    {
        case pq_oid_arr_bool   : case pq_oid_arr_int2   : case pq_oid_arr_int4  :
        case pq_oid_arr_int8   : case pq_oid_arr_float4 : case pq_oid_arr_float8:
        case pq_oid_arr_numeric: case pq_oid_arr_char   : case pq_oid_arr_text  :
        case pq_oid_arr_bpchar : case pq_oid_arr_varchar: case pq_oid_arr_bytea :
        case pq_oid_arr_json   : case pq_oid_arr_jsonb  :
            return true;
        default:
            return false;
    }
}


template<pq_oid_e Id, pq_fmt_e Fmt = pq_fmt_binary>
struct pq_param_write_result
    : std::span<char const, Fmt == pq_fmt_binary ? std::dynamic_extent : 0>
{
    static constexpr pq_oid_e   oid = Id;
    static constexpr pq_fmt_e fmt = Fmt;

    using base = std::span<char const, Fmt == pq_fmt_binary ? std::dynamic_extent : 0>;
    using base::base;
};


template<class T, pq_oid_e ArrOid, pq_oid_e ElmOid, size_t ElmSize, class WriteElm, class GetElmSize>
struct arr1d_pq_param_writer
{
    // header: dimCnt | hasnullFlag | elmOid | <elmCntInEachDim + firstElmIndexOfEachDim> ...
    //         each of them is int32_t
    static constexpr size_t hdr_size = 5 * sizeof(int32_t);

    using buf_type = DSK_CONDITIONAL_T(! (_tuple_like_<T> && ElmSize),
                                       vector<char>,
                                       std::array<char, hdr_size + elm_count<T> * (sizeof(int32_t) + ElmSize)>);
    buf_type buf;

    DSK_NO_UNIQUE_ADDR WriteElm writeElm;
    DSK_NO_UNIQUE_ADDR GetElmSize getElmSize;

    constexpr explicit arr1d_pq_param_writer(auto&& w, auto&& g)
        : writeElm(DSK_FORWARD(w)), getElmSize(DSK_FORWARD(g))
    {}

    constexpr auto operator()(auto& v)
    {
        if constexpr(! _tuple_like_<T> && ElmSize)
        {
            rescale_buf(buf, hdr_size + std::size(v) * (sizeof(int32_t) + ElmSize));
        }
        else if constexpr(! _tuple_like_<T>)
        {
            size_t n = 0;
            
            for(auto& e : v)
            {
                n += getElmSize(const_cast<DSK_NO_CVREF_T(e)&>(e));
            }

            rescale_buf(buf, hdr_size + std::size(v) * sizeof(int32_t) + n);
        }

        char* d = buf.data();

        write_int32(d, 1); // dimCnt
        write_int32(d, 0); // hasnullFlag
        write_int32(d, static_cast<Oid>(ElmOid)); // elmOid
        write_int32(d, std::size(v)); // elmCntInEachDim
        write_int32(d, 1); // firstElmIndexOfEachDim, postgres uses 1-based arrays which maps to 0 for c-array.

        for(auto e : v)
        {
            if constexpr(ElmSize) write_int32(d, ElmSize);
            else                  write_int32(d, getElmSize(const_cast<DSK_NO_CVREF_T(e)&>(e)));

            d = writeElm(d, const_cast<DSK_NO_CVREF_T(e)&>(e));
        }

        return pq_param_write_result<ArrOid>(buf);
    }

    static constexpr void write_int32(char*& d, std::integral auto t) noexcept
    {
        store_be(d, static_cast<int32_t>(t));
        d += sizeof(int32_t);
    }
};

template<pq_oid_e ArrOid, pq_oid_e ElmOid, size_t ElmSize>
constexpr auto make_arr1d_pq_param_writer(auto& v, auto&& writeElm)
{
    static_assert(ElmSize);

    return arr1d_pq_param_writer
    <
        DSK_NO_CVREF_T(v), ArrOid, ElmOid, ElmSize,
        DSK_DECAY_T(writeElm), blank_t
    >(
        DSK_FORWARD(writeElm), blank_c
    );
}

template<pq_oid_e ArrOid, pq_oid_e ElmOid>
constexpr auto make_arr1d_pq_param_writer(auto& v, auto&& getElmSize, auto&& writeElm)
{
    return arr1d_pq_param_writer
    <
        DSK_NO_CVREF_T(v), ArrOid, ElmOid, 0,
        DSK_DECAY_T(writeElm), DSK_DECAY_T(getElmSize)
    >(
        DSK_FORWARD(writeElm), DSK_FORWARD(getElmSize)
    );
}


inline constexpr struct get_pq_param_writer_cpo
{
    constexpr auto operator()(auto& v) const
    requires(requires{
        dsk_get_pq_param_writer(*this, const_cast<DSK_NO_CVREF_T(v)&>(v)); })
    {
        return dsk_get_pq_param_writer(*this, const_cast<DSK_NO_CVREF_T(v)&>(v));
    }
} get_pq_param_writer;


template<class T>
inline constexpr pq_oid_e pq_param_oid = decltype(get_pq_param_writer(std::declval<T&>())(std::declval<T&>()))::oid;

template<class T>
inline constexpr pq_fmt_e pq_param_fmt = decltype(get_pq_param_writer(std::declval<T&>())(std::declval<T&>()))::fmt;


constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, _optional_ auto& v)
{
    return [writer = std::optional<decltype(get_pq_param_writer(*v))>()](auto& v) mutable
    {
        if(v)
        {
            writer = get_pq_param_writer(*v);
            return (*writer)(*v);
        }
        else
        {
            return pq_param_write_result<pq_param_oid<decltype(*v)>>();
        }
    };
}

constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, std::nullptr_t&)
{
    return [](std::nullptr_t)
    {
        return pq_param_write_result<pq_oid_invalid>();
    };
}


struct pq_char
{
    char v;

    constexpr pq_char(char c) noexcept : v(c) {}
    constexpr operator char() const noexcept { return v; }
};


template<class T>
concept _pq_arithmetic_ = _arithmetic_<T> || _no_cvref_same_as_<T, pq_char>;


template<class T>
struct arithmetic_pq_param_info_t
{
    using type = T;
    pq_oid_e elmOid, arrOid;
};

template<class T>
constexpr auto arithmetic_pq_param_info = [](this auto self)
{
    static_assert(! _same_as_<char, int8_t>);
    static_assert(! _same_as_<char, uint8_t>);

         if constexpr(_same_as_<T, bool    >) return arithmetic_pq_param_info_t<char   >{  pq_oid_bool,   pq_oid_arr_bool};
    else if constexpr(_same_as_<T, char    >) return arithmetic_pq_param_info_t<char   >{  pq_oid_char,   pq_oid_arr_char};
    else if constexpr(_same_as_<T, pq_char >) return arithmetic_pq_param_info_t<char   >{  pq_oid_char,   pq_oid_arr_char};
    else if constexpr(_same_as_<T,  int8_t >) return arithmetic_pq_param_info_t<int16_t>{  pq_oid_int2,   pq_oid_arr_int2};
    else if constexpr(_same_as_<T, uint8_t >) return arithmetic_pq_param_info_t<int16_t>{  pq_oid_int2,   pq_oid_arr_int2};
    else if constexpr(_same_as_<T,  int16_t>) return arithmetic_pq_param_info_t<int16_t>{  pq_oid_int2,   pq_oid_arr_int2};
    else if constexpr(_same_as_<T, uint16_t>) return arithmetic_pq_param_info_t<int32_t>{  pq_oid_int4,   pq_oid_arr_int4};
    else if constexpr(_same_as_<T,  int32_t>) return arithmetic_pq_param_info_t<int32_t>{  pq_oid_int4,   pq_oid_arr_int4};
    else if constexpr(_same_as_<T, uint32_t>) return arithmetic_pq_param_info_t<int64_t>{  pq_oid_int8,   pq_oid_arr_int8};
    else if constexpr(_same_as_<T,  int64_t>) return arithmetic_pq_param_info_t<int64_t>{  pq_oid_int8,   pq_oid_arr_int8};
    else if constexpr(_same_as_<T, float   >) return arithmetic_pq_param_info_t<float  >{pq_oid_float4, pq_oid_arr_float4};
    else if constexpr(_same_as_<T, double  >) return arithmetic_pq_param_info_t<double >{pq_oid_float8, pq_oid_arr_float8};
    else
        if constexpr(std::is_enum_v<T> && ! std::is_scoped_enum_v<T>)
        {
            return self.template operator()<std::underlying_type_t<T>>();
        }
    else
        static_assert(false, "unsupported type");
}();


constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, _pq_arithmetic_ auto& v)
{
    constexpr auto info = arithmetic_pq_param_info<DSK_NO_CVREF_T(v)>;

    using type = typename decltype(info)::type;
    using result_type = pq_param_write_result<info.elmOid>; // clang workaround

    return [buf = std::array<char, sizeof(type)>()](type v) mutable
    {
        store_be(buf.data(), v);
        return result_type(buf);
    };
}

constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, std::ranges::range auto& v)
requires(
    ! _byte_str_<decltype(v)>
    && _pq_arithmetic_<unchecked_range_value_t<decltype(v)>>)
{
    constexpr auto info = arithmetic_pq_param_info<unchecked_range_value_t<decltype(v)>>;

    using type = typename decltype(info)::type;

    return make_arr1d_pq_param_writer<info.arrOid, info.elmOid, sizeof(type)>
    (
        v,
        [](char* d, type e)
        {
            store_be(d, e);
            return d + sizeof(e);
        }
    );
}


constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, _byte_str_ auto&)
{
    return [](auto& v)
    {
        return pq_param_write_result<pq_oid_text>(str_data<char>(v), str_size(v));
    };
}

constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, std::ranges::range auto& v)
requires(
    _byte_str_<unchecked_range_value_t<decltype(v)>>)
{
    return make_arr1d_pq_param_writer<pq_oid_arr_text, pq_oid_text>
    (
        v,
        [](auto& e) { return str_size(e); },
        [](char* d, auto& e) { return append_str(d, e); }
    );
}


constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, _buf_of_<std::byte> auto&)
{
    return [](auto& v)
    {
        return pq_param_write_result<pq_oid_bytea>(as_buf_of<char>(v));
    };
}

constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, std::ranges::range auto& v)
requires(
    _buf_of_<unchecked_range_value_t<decltype(v)>, std::byte>
)
{
    return make_arr1d_pq_param_writer<pq_oid_arr_bytea, pq_oid_bytea>
    (
        v,
        [](auto& e) { return buf_size(e); },
        [](char* d, auto& e) { return append_buf(d, as_buf_of<char>(e)); }
    );
}


// text parameter
template<_byte_cstr_ S>
struct pq_tparam
{
    S v;
};

template<_byte_cstr_ S>
pq_tparam(S&& s) -> pq_tparam<lref_or_val_t<S>>;


template<class S>
constexpr auto dsk_get_pq_param_writer(get_pq_param_writer_cpo, pq_tparam<S>&)
{
    return [](auto& v)
    {
        return pq_param_write_result<pq_oid_invalid, pq_fmt_text>(cstr_data<char>(v.v), 0);
    };
}


template<class... Params>
struct pq_params_builder
{
    static constexpr size_t count = sizeof...(Params);
    Oid          oids[count];
    int          fmts[count];
    char const*  vals[count];
    int          lens[count];

    tuple<
        decltype(get_pq_param_writer(std::declval<Params&>()))...
    > writers;

    explicit pq_params_builder(auto&... params)
        : writers(get_pq_param_writer(params)...)
    {
        static_assert(sizeof...(params) == count);

        foreach_elms(fwd_as_tuple(params...), [this]<auto I>(auto& param)
        {
            auto r = writers[idx_c<I>](param);

            oids[I] = r.oid;
            fmts[I] = r.fmt;
            vals[I] = r.data();
            lens[I] = static_cast<int>(r.size());
        });
    }
};

template<>
struct pq_params_builder<>
{
    static constexpr size_t       count   = 0;
    static constexpr Oid  const*  oids   = nullptr;
    static constexpr char const** vals  = nullptr;
    static constexpr int  const*  lens = nullptr;
    static constexpr int  const*  fmts = nullptr;
};


constexpr auto build_pq_params(auto&... params)
{
    return pq_params_builder<DSK_NO_CVREF_T(params)...>(params...);
}


// pq_fmt_text || is_textual
inline constexpr struct from_pq_textual_cpo
{
    constexpr auto operator()(char const* d, int n, auto& v) const
    requires(requires{
        dsk_from_pq_textual(*this, d, n, v); })
    {
        return dsk_from_pq_textual(*this, d, n, v);
    }
} from_pq_textual;


constexpr errc dsk_from_pq_textual(from_pq_textual_cpo, char const* d, int n, bool& v)
{
    std::string_view s(d, n);

         if(s == "true" ) v = true;
    else if(s == "false") v = false;
    else
        return errc::invalid_input;

    return {};
}

constexpr errc dsk_from_pq_textual(from_pq_textual_cpo, char const* d, int n, char& v)
{
    if(n == 1)
    {
        v = *d;
        return {};
    }

    return errc::size_mismatch;
}

constexpr errc dsk_from_pq_textual(from_pq_textual_cpo, char const* d, int n, pq_char& v)
{
    if(n == 1)
    {
        v.v = *d;
        return {};
    }

    return errc::size_mismatch;
}

constexpr auto dsk_from_pq_textual(from_pq_textual_cpo, char const* d, int n, _arithmetic_ auto& v)
{
    return from_whole_str(std::string_view(d, n), v);
}

constexpr void dsk_from_pq_textual(from_pq_textual_cpo, char const* d, int n, _byte_str_ auto& v)
{
    assign_str(v, std::string_view(d, n));
}


inline constexpr struct from_pq_binary_cpo
{
    constexpr auto operator()(pq_oid_e t, char const* d, int n, auto& v) const
    requires(requires{
        dsk_from_pq_binary(*this, t, d, n, v); })
    {
        DSK_ASSERT(n >= 0);
        return dsk_from_pq_binary(*this, t, d, n, v);
    }
} from_pq_binary;


constexpr errc dsk_from_pq_binary(from_pq_binary_cpo, pq_oid_e t, char const* d, int n, _pq_arithmetic_ auto& v)
{
    auto do_read = [&]<class T>(std::type_identity<T>)
    {
        if(n == sizeof(T)) return cvt_num(load_be<T>(d), v);
        else               return errc::size_mismatch;
    };

    switch(t)
    {
        case   pq_oid_bool: return do_read(type_c<   char>);
        case   pq_oid_char: return do_read(type_c<   char>);
        case   pq_oid_int2: return do_read(type_c<int16_t>);
        case   pq_oid_int4: return do_read(type_c<int32_t>);
        case   pq_oid_int8: return do_read(type_c<int64_t>);
        case pq_oid_float4: return do_read(type_c<  float>);
        case pq_oid_float8: return do_read(type_c< double>);
        default:
            return errc::type_mismatch;
    }
}

constexpr errc dsk_from_pq_binary(from_pq_binary_cpo, pq_oid_e t, char const* d, int n, _byte_str_ auto& v)
{
    if(is_textual(t))
    {
        assign_str(v, std::string_view(d, n));
        return {};
    }

    return errc::type_mismatch;
}

constexpr void dsk_from_pq_binary(from_pq_binary_cpo, pq_oid_e, char const* d, int n, _buf_of_<std::byte> auto& v)
{
    assign_buf(v, alias_cast<buf_val_t<decltype(v)>>(d), n);
}

constexpr error_code dsk_from_pq_binary(from_pq_binary_cpo, pq_oid_e t, char const* d, int n, std::ranges::range auto& v)
requires(
    ! _byte_str_<decltype(v)>
    && (_pq_arithmetic_<unchecked_range_value_t<decltype(v)>>
        || _byte_str_<unchecked_range_value_t<decltype(v)>>
        || _buf_of_<unchecked_range_value_t<decltype(v)>, std::byte>))
{
    if(! is_array(t))
    {
        return errc::type_mismatch;
    }

    if(n < static_cast<int>(5 * sizeof(int32_t)))
    {
        return errc::size_mismatch;
    }

    auto end = d + n;

    auto dimCnt = load_be<int32_t>(d);
    d += sizeof(int32_t);
    //auto hasnullFlag = load_be<int32_t>(d);
    d += sizeof(int32_t);
    auto elmOid = load_be<int32_t>(d);
    d += sizeof(int32_t);
    auto elmCnt = load_be<int32_t>(d);
    d += sizeof(int32_t);
    //auto firstElmIndex = load_be<int32_t>(d);
    d += sizeof(int32_t);

    if(dimCnt != 1 || elmOid < 0 || elmCnt < 0)
    {
        return errc::invalid_input;
    }

    auto read_int32 = [&]() -> expected<int32_t, errc>
    {
        if(d + sizeof(int32_t) < end)
        {
            auto p = d;
            d += sizeof(int32_t);
            return load_be<int32_t>(p);
        }

        return errc::size_mismatch;
    };

    if constexpr(requires{ v.push_back(std::declval<unchecked_range_value_t<decltype(v)>>()); })
    {
        v.clear();
        
        if constexpr(requires{ v.reserve(elmCnt); })
        {
            v.reserve(elmCnt);
        }

        for(int32_t i = 0; i < elmCnt; ++i)
        {
            unchecked_range_value_t<decltype(v)> ev;

            DSK_E_TRY(auto en, read_int32());

            if(en < 0 || d + en > end)
            {
                return errc::invalid_size;
            }

            DSK_U_TRY_ONLY(from_pq_binary(static_cast<pq_oid_e>(elmOid), d, en, ev));
            d += en;

            v.push_back(mut_move(ev));
        }
    }
    else // fix size range
    {
        if(elmCnt != std::size(v))
        {
            return errc::size_mismatch;
        }

        for(auto& ev : v)
        {
            DSK_E_TRY(auto en, read_int32());

            if(en < 0 || d + en > end)
            {
                return errc::invalid_size;
            }

            DSK_U_TRY_ONLY(from_pq_binary(static_cast<pq_oid_e>(elmOid), d, en, ev));
            d += en;
        }
    }

    if(d != end)
    {
        return errc::size_mismatch;
    }

    return {};
}



} // namespace dsk
