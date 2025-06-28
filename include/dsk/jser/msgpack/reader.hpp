#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/endian.hpp>
#include <dsk/jser/cpo.hpp>
#include <dsk/jser/msgpack/fmt.hpp>
#include <dsk/jser/msgpack/err.hpp>
#include <cstring>


namespace dsk{


template<class T>
using msgpack_expected = expected<T, msgpack_errc>;


template<unsigned Opts>
class msgpack_value
{
    static constexpr bool useLittleEndian = Opts & msgpack_opt_little_endian;
    static constexpr bool usePackedArr = Opts & msgpack_opt_packed_arr;
    static constexpr auto endian = useLittleEndian ? endian_order::little : endian_order::big;

    uint8_t const*       _p = nullptr; // current position.
    uint8_t const* const _e = nullptr; // end of buf

public:
    constexpr explicit msgpack_value(_byte_buf_ auto const& b, size_t offset = 0) noexcept
        : _p(buf_data<uint8_t>(b) + offset), _e(_p + buf_size(b))
    {
        DSK_ASSERT(buf_size(b) >= offset);
    }

    constexpr bool has_more(size_t n) const noexcept
    {
        return _p + n <= _e;
    }

    template<class T>
    constexpr msgpack_expected<T> read() noexcept
    {
        if constexpr(sizeof(T) == 1) { if(_p < _e            ) return static_cast<T>(*(_p++)); }
        else                         { if(has_more(sizeof(T))) return load_xe<endian, T>(std::exchange(_p, _p + sizeof(T))); }

        return msgpack_errc::not_enough_input_to_read;
    }

    constexpr msgpack_errc read(void* d, size_t n) noexcept
    {
        if(has_more(n))
        {
            std::memcpy(d, _p, n);
            _p += n;
            return {};
        }

        return msgpack_errc::not_enough_input_to_read;
    }

    constexpr msgpack_errc read_buf(size_t n, _byte_buf_ auto& d) noexcept
    {
        if(has_more(n))
        {
            assign_buf(d, alias_cast<buf_val_t<decltype(d)>>(_p), n);
            _p += n;
            return {};
        }

        return msgpack_errc::not_enough_input_to_read;
    }

    constexpr uint8_t const* cur_pos() const noexcept { return _p; }
    
    constexpr msgpack_errc advance(size_t n) noexcept
    {
        if(has_more(n)) { _p += n; return {}; }
        else            return msgpack_errc::not_enough_input_to_read;
    }

    constexpr msgpack_expected<uint8_t> peek_uint8() noexcept
    {
        if(_p < _e) return *_p;
        else        return msgpack_errc::not_enough_input_to_read;
    }

    template<_unsigned_ T>
    constexpr msgpack_expected<T> uint() noexcept
    {
        static_assert(sizeof(T) <= 8);

        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t <= 0x7f) return t;
                                     if(t == msgpack_posi8 ::tag) return read<uint8_t >();
        if constexpr(sizeof(T) >= 2) if(t == msgpack_posi16::tag) return read<uint16_t>();
        if constexpr(sizeof(T) >= 4) if(t == msgpack_posi32::tag) return read<uint32_t>();
        if constexpr(sizeof(T) == 8) if(t == msgpack_posi64::tag) return read<uint64_t>();

        return msgpack_errc::tag_mismatch;
    }

    // We assume the value to read is encoded using the same type as T, except int64_t.
    // int64_t supports decoding from any encoding integral type.
    // You can use int64_t as general integral type.
    template<_signed_integral_ T>
    constexpr msgpack_expected<T> int_() noexcept
    {
        static_assert(sizeof(T) <= 8);
        
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t <= 0x7f) return t;
        if(t >= 0xe0) return static_cast<int8_t>(t);

        if(t == msgpack_negi8::tag) return read< int8_t>();
        if(t == msgpack_posi8::tag) return read<uint8_t>();

        if constexpr(sizeof(T) >= 2)
        {
            if(t == msgpack_negi16::tag) return read< int16_t>();
            if(t == msgpack_posi16::tag) return read<uint16_t>();
        }

        if constexpr(sizeof(T) >= 4)
        {
            if(t == msgpack_negi32::tag) return read< int32_t>();
            if(t == msgpack_posi32::tag) return read<uint32_t>();
        }

        if constexpr(sizeof(T) == 8)
        {
            if(t == msgpack_negi64::tag) return read< int64_t>();
            if(t == msgpack_posi64::tag)
            {
                DSK_E_TRY(T v, read<uint64_t>());
                if(v >= 0) return v;
                else       return msgpack_errc::posi_overflow;
            }
        }

        return msgpack_errc::tag_mismatch;
    }

    constexpr msgpack_expected<float> float_() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t == msgpack_float::tag) return read<float>();
        else                        return msgpack_errc::tag_mismatch;
    }

    // Supports decoding from both double and float.
    // You can use double as general floating point type.
    constexpr msgpack_expected<double> double_() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t == msgpack_double::tag) return read<double>();
        if(t == msgpack_float ::tag) return read<float >();
        else                         return msgpack_errc::tag_mismatch;
    }

    constexpr msgpack_errc nil() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t == msgpack_nil) return {};
        else                 return msgpack_errc::tag_mismatch;
    }

    // only consumed when it's actually nil.
    constexpr msgpack_expected<bool> check_nil() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t == msgpack_nil){       return  true; }
        else                { --_p; return false; }
    }

    constexpr auto null() noexcept { return nil(); }
    constexpr auto check_null() noexcept { return check_nil(); }

    constexpr msgpack_expected<bool> bool_() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        if(t == msgpack_true ) return true;
        if(t == msgpack_false) return false;
        
        return msgpack_errc::tag_mismatch;
    }

    template<_arithmetic_ T>
    constexpr msgpack_expected<T> arithmetic() noexcept
    {
             if constexpr(_same_as_<T,   bool>) return   bool_();
        else if constexpr(_same_as_<T,  float>) return  float_();
        else if constexpr(_same_as_<T, double>) return double_();
        else if constexpr(       _unsigned_<T>) return uint<T>();
        else if constexpr(_signed_integral_<T>) return int_<T>();
        else
            static_assert(false, "unsupported type");
    }

    template<_arithmetic_ T>
    constexpr msgpack_errc get_arithmetic(T& t) noexcept
    {
        DSK_E_TRY(t, arithmetic<T>());
        return {};
    }

    constexpr msgpack_errc get_str(_byte_buf_ auto& d) noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        size_t n = 0;

             if(     (t & 0xe0) == 0xa0) n = t & 0x1f;
        else if(t == msgpack_str8 ::tag) { DSK_E_TRY(n, read<uint8_t >()); }
        else if(t == msgpack_str16::tag) { DSK_E_TRY(n, read<uint16_t>()); }
        else if(t == msgpack_str32::tag) { DSK_E_TRY(n, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        return read_buf(n, d);
    }

    constexpr msgpack_errc get_bin(_byte_buf_ auto& d) noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        size_t n = 0;

             if(t == msgpack_bin8 ::tag) { DSK_E_TRY(n, read<uint8_t >()); }
        else if(t == msgpack_bin16::tag) { DSK_E_TRY(n, read<uint16_t>()); }
        else if(t == msgpack_bin32::tag) { DSK_E_TRY(n, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        return read_buf(n, d);
    }

    constexpr msgpack_errc get_bin(_byte_ auto* d, size_t s) noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        size_t n = 0;

             if(t == msgpack_bin8 ::tag) { DSK_E_TRY(n, read<uint8_t >()); }
        else if(t == msgpack_bin16::tag) { DSK_E_TRY(n, read<uint16_t>()); }
        else if(t == msgpack_bin32::tag) { DSK_E_TRY(n, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        if(n == s) return read(d, n);
        else       return msgpack_errc::buf_mismatch;
    }


    template<_byte_buf_ T = std::string_view>
    constexpr msgpack_expected<T> str() noexcept
    {
        T t;
        DSK_E_TRY_ONLY(get_str(t));
        return t;
    }

    template<_byte_buf_ T = std::span<char const>>
    constexpr msgpack_expected<T> bin() noexcept
    {
        T t;
        DSK_E_TRY_ONLY(get_bin(t));
        return t;
    }

    constexpr msgpack_errc skip(size_t cnt = 1) noexcept
    {
        while(cnt--)
        {
            DSK_E_TRY_FWD(t, read<uint8_t>());

            size_t n = 0;

            // posi fixint  negi fixint
            if(t <= 0x7f || t >= 0xe0 || is_oneof(t, msgpack_nil, msgpack_false, msgpack_true)) continue;

            else if(is_oneof(t, msgpack_posi8 ::tag, msgpack_negi8 ::tag)) n = 1;
            else if(is_oneof(t, msgpack_posi16::tag, msgpack_negi16::tag)) n = 2;

            else if(is_oneof(t, msgpack_posi32::tag, msgpack_negi32::tag, msgpack_float ::tag)) n = 4;
            else if(is_oneof(t, msgpack_posi64::tag, msgpack_negi64::tag, msgpack_double::tag)) n = 8;
            
            else if((t & 0xe0) == 0xa0)    n = (t & 0x1f)    ; // fixstr
            else if((t & 0xf0) == 0x90) cnt += (t & 0x0f)    ; // fixarray
            else if((t & 0xf0) == 0x80) cnt += (t & 0x0f) * 2; // fixmap

            else if(is_oneof(t, msgpack_str8 ::tag, msgpack_bin8 ::tag)) { DSK_E_TRY(n, read<uint8_t >()); }
            else if(is_oneof(t, msgpack_str16::tag, msgpack_bin16::tag)) { DSK_E_TRY(n, read<uint16_t>()); }
            else if(is_oneof(t, msgpack_str32::tag, msgpack_bin32::tag)) { DSK_E_TRY(n, read<uint32_t>()); }

            else if(t == msgpack_arr16::tag) { DSK_E_TRY_FWD(m, read<uint16_t>()); cnt += m;}
            else if(t == msgpack_map16::tag) { DSK_E_TRY_FWD(m, read<uint16_t>()); cnt += m * 2; }
            else if(t == msgpack_arr32::tag) { DSK_E_TRY_FWD(m, read<uint32_t>()); cnt += m;}
            else if(t == msgpack_map32::tag) { DSK_E_TRY_FWD(m, read<uint32_t>()); cnt += m * 2; }

            else if(t == msgpack_ext8 ::tag) { DSK_E_TRY(n, read<uint8_t >()); ++n; }
            else if(t == msgpack_ext16::tag) { DSK_E_TRY(n, read<uint16_t>()); ++n; }
            else if(t == msgpack_ext32::tag) { DSK_E_TRY(n, read<uint32_t>()); ++n; }

            else if(t == msgpack_tag_fixext1 ) n = 1 +  1;
            else if(t == msgpack_tag_fixext2 ) n = 1 +  2;
            else if(t == msgpack_tag_fixext4 ) n = 1 +  4;
            else if(t == msgpack_tag_fixext8 ) n = 1 +  8;
            else if(t == msgpack_tag_fixext16) n = 1 + 16;

            else
            {
                return msgpack_errc::tag_mismatch;
            }

            if(n)
            {
                _p += n;
            }
        }

        return {};
    }

    struct iterator
    {
        using is_msgpack_iter = void;

        msgpack_value& _v;
        uint32_t _n;
        
        constexpr explicit operator bool() const noexcept
        {
            return _n;
        }

        // not actually skip value, must read 1(for arr) or 2(for map) values before calling this.
        constexpr iterator& operator++() noexcept
        {
            --_n;
            return *this;
        }

        // for arr, must read exactly 1 item from returned msgpack_value;
        // for map, must read exactly 2 items from returned msgpack_value;
        constexpr msgpack_value& operator*() noexcept
        {
            return _v;
        }

        // shouldn't use this iterator after calling this method.
        constexpr msgpack_errc skip_remains() noexcept
        {
            return _v.skip(_n);
        }
    };

    struct loop_iterator
    {
        using is_msgpack_loop_iter = void;

        msgpack_value& _v;
        uint32_t const _n;
        uint8_t const* _p = _v._p;
        uint32_t _i = 0; // absolute pos
        uint32_t _j = 0; // relative pos

        // start a new loop relative to current position.
        constexpr void start() noexcept
        {
            DSK_ASSERT(_n);
            _j = 0;
        }

        // must use the returned non null value.
        // for arr, must read exactly 1 item;
        // for map, must read exactly 2 items;
        constexpr optional<msgpack_value&> next() noexcept
        {
            if(_j++ < _n)
            {
                if(_i++ == _n)
                {
                    _i = 0;
                    _v._p = _p;
                }

                return _v;
            }

            return std::nullopt;
        }

        // shouldn't use this iterator after calling this method.
        constexpr msgpack_errc skip_remains() noexcept
        {
            DSK_ASSERT(_n >= _i);
            return _v.skip(_n - _i);
        }
    };

    template<bool Loop>
    using iter_t = std::conditional_t<Loop, loop_iterator, iterator>;

    template<bool Loop = false>
    constexpr msgpack_expected<iter_t<Loop>> map_iter() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        uint32_t n = 0;

             if(     (t & 0xf0) == 0x80) n = t & 0x0f;
        else if(t == msgpack_map16::tag) { DSK_E_TRY(n, read<uint16_t>()); }
        else if(t == msgpack_map32::tag) { DSK_E_TRY(n, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        return {expect, *this, n};
    }

    template<bool Loop = false>
    constexpr msgpack_expected<iter_t<Loop>> arr_iter() noexcept
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());

        uint32_t n = 0;

             if(     (t & 0xf0) == 0x90) n = t & 0x0f;
        else if(t == msgpack_arr16::tag) { DSK_E_TRY(n, read<uint16_t>()); }
        else if(t == msgpack_arr32::tag) { DSK_E_TRY(n, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        return {expect, *this, n};
    }

    constexpr auto map_loop_iter() noexcept { return map_iter<true>(); }
    constexpr auto arr_loop_iter() noexcept { return arr_iter<true>(); }

    template<class T>
    struct packed_arr_value
    {
        msgpack_value& _v;
        uint32_t _n;

        packed_arr_value(msgpack_value& v, uint32_t n) noexcept
            : _v(v), _n(n)
        {}

        constexpr uint32_t count() const noexcept { return _n; }

        constexpr msgpack_errc memcpy_to(_buf_of_<T> auto&& b) noexcept requires(endian == endian_order::native)
        {
            DSK_ASSERT(_n >= buf_size(b));

            DSK_E_TRY_ONLY(_v.read(buf_data(b), _n * sizeof(T)));

            _n -= buf_size(b);
            return {};
        }

        constexpr msgpack_errc get_arithmetic(auto& t) noexcept
        {
            DSK_ASSERT(_n);

            DSK_E_TRY(t, _v.read<T>());
            --_n;
            return {};
        }

        // shouldn't use this after calling this method.
        constexpr msgpack_errc skip_arr_remains() noexcept
        {
            return _v.advance(_n * sizeof(T));
        }

        /// also act as arr_iter

        using is_jpacked_arr_iter = void;

        auto& arr_iter() noexcept { return *this; }

        constexpr explicit operator bool() const noexcept { return _n; }
        constexpr auto& operator++() noexcept { return *this; }
        constexpr auto& operator*() noexcept { return *this; }
    };

    template<class T>
    constexpr msgpack_expected<packed_arr_value<T>> packed_arr() noexcept requires(usePackedArr)
    {
        DSK_E_TRY_FWD(t, read<uint8_t>());
        
        uint32_t nByte = 0;

             if(t == msgpack_ext8 ::tag) { DSK_E_TRY(nByte, read<uint8_t >()); }
        else if(t == msgpack_ext16::tag) { DSK_E_TRY(nByte, read<uint16_t>()); }
        else if(t == msgpack_ext32::tag) { DSK_E_TRY(nByte, read<uint32_t>()); }
        else
            return msgpack_errc::tag_mismatch;

        if(nByte % sizeof(T) != 0)
        {
            return msgpack_errc::invalid_ext_size;
        }

        DSK_E_TRY_FWD(vt, read<uint8_t>());

        if(vt != msgpack_packed_arr_of<T>::tag)
        {
            return msgpack_errc::ext_type_mismatch;
        }

        return packed_arr_value<T>(*this, nByte / sizeof(T));
    }
};


template<unsigned Opts> void _msgpack_value_impl(msgpack_value<Opts>&);
template<class T> concept _msgpack_value_ = requires(std::remove_cvref_t<T>& t){ dsk::_msgpack_value_impl(t); };
template<class T> concept _msgpack_iter_  = requires{ typename std::remove_cvref_t<T>::is_msgpack_iter; };
template<class T> concept _msgpack_loop_iter_  = requires{ typename std::remove_cvref_t<T>::is_msgpack_loop_iter; };


constexpr auto dsk_get_jstr_noescape(get_jstr_noescape_cpo, _msgpack_value_ auto& v, auto& t) noexcept
{
    return v.get_str(t);
}

constexpr auto dsk_get_jbinary(get_jbinary_cpo, _msgpack_value_ auto& v, auto& t) noexcept
{
    if constexpr(_byte_buf_<decltype(t)>)
    {
        return v.get_bin(t);
    }
    else
    {
        auto&& b = as_buf_of<char>(t);
        return v.get_bin(buf_data(b), buf_size(b));
    }
}

constexpr auto dsk_get_jobj(get_jobj_cpo, _msgpack_value_ auto& v) noexcept
{
    return v.map_iter();
}

constexpr auto& dsk_get_jobj(get_jobj_cpo, _msgpack_iter_ auto& v) noexcept
{
    return DSK_FORWARD(v);
}

template<class Key>
constexpr error_code dsk_foreach_jfld(foreach_jfld_cpo<Key>, _msgpack_iter_ auto& it, auto&& h)
{
    for(; it; ++it)
    {
        auto& v = *it;
        Key k;

             if constexpr(       _str_<Key>) { DSK_E_TRY_ONLY(v.get_str(k)); }
        else if constexpr(_arithmetic_<Key>) { DSK_E_TRY(k, v.template arithmetic<Key>()); }
        else
            static_assert(false, "unsupported type");

        DSK_E_TRY_ONLY(h(mut_move(k), v));
    }

    return {};
}

constexpr auto dsk_get_jval_finder(get_jval_finder_cpo, _msgpack_value_ auto& v) noexcept
{
    return v.map_loop_iter();
}

constexpr auto& dsk_get_jval_finder(get_jval_finder_cpo, _msgpack_loop_iter_ auto& v) noexcept
{
    return v;
}

constexpr expected<int> dsk_foreach_found_jval(foreach_found_jval_cpo, _msgpack_loop_iter_ auto& it, auto& mks, auto&& h)
{
    static_assert(mks.count);

    int unmatched = mks.count;

    it.start();

    while(auto ov = it.next())
    {
        auto& f = *ov;

        DSK_E_TRY_FWD(fk, DSK_CONDITIONAL_V(_str_<decltype(jkey_uint_or_str(mks[idx_c<0>][idx_c<1>]))>,
                                                f.str(),
                                                f.template uint<jkey_uint_t>()));

        auto ec = foreach_elms_until_err(mks, [&]<auto I>(auto& mk) -> error_code
        {
            auto&[matched, k] = mk;

            if(! matched && fk == jkey_uint_or_str(k))
            {
                DSK_E_TRY_ONLY(h.template operator()<I>(f));
                    
                matched = true;

                --unmatched;

                return errc::none_err; // for matched
            }

            return {};
        });

        if(! ec)
        {
            DSK_E_TRY_ONLY(f.skip()); // no match for fk, so skip fv
            continue;
        }

        if(ec != errc::none_err)
        {
            return ec;
        }

        if(! unmatched)
        {
            return 0;
        }
    }

    return unmatched;
}

template<template<class...> class L, class... T>
constexpr msgpack_expected<unsigned> dsk_get_jval_matched_type(get_jval_matched_type_cpo,
                                                               _msgpack_value_ auto& v, L<T...> const&) noexcept
{
    static_assert(sizeof...(T));
    constexpr auto ts = type_list<T...>;
    constexpr auto nInt = ct_count(ts, DSK_TYPE_EXP_OF(U, _integral_<U> && ! _same_as_<U, bool>));
    constexpr auto nFp  = ct_count(ts, DSK_TYPE_EXP_OF(U, _fp_<U>));
    static_assert(nInt <= 1 && nFp <= 1, "At most 1 integral and 1 floating point types are allowed for number.");
    // NOTE: for numbers, it will decode to same type, if same type is used in encoding.

    DSK_E_TRY_FWD(t, v.peek_uint8());

    return foreach_ct_elms_until<make_index_array<sizeof...(T)>()>
    (
        [&]<unsigned I>() -> msgpack_expected<unsigned>
        {
            using e_t = type_pack_elm<I, T...>;

            if constexpr(_same_as_<e_t, std::monostate>)
            {
                static_assert(I == 0);
                if(t == msgpack_nil) return I;
            }
            else if constexpr(_same_as_<e_t, bool>)
            {
                if(t == msgpack_true || t == msgpack_false) return I;
            }
            else if constexpr(_fp_<e_t>)
            {
                if constexpr(sizeof(e_t) <= sizeof(float)) { if(                            t == msgpack_float::tag) return I; }
                else                                       { if(t == msgpack_double::tag || t == msgpack_float::tag) return I; }
            }
            else if constexpr(_unsigned_<e_t>)
            {
                if(t <= 0x7f) return I;

                     if constexpr(sizeof(e_t) == 1) { if(t == msgpack_posi8::tag                            ) return I; }
                else if constexpr(sizeof(e_t) == 2) { if(t == msgpack_posi8::tag || t == msgpack_posi16::tag) return I; }
                else if constexpr(sizeof(e_t) == 4) { if(msgpack_posi8::tag <= t && t <= msgpack_posi32::tag) return I; }
                else                                { if(msgpack_posi8::tag <= t && t <= msgpack_posi64::tag) return I; }
            }
            else if constexpr(_signed_integral_<e_t>)
            {
                if(t <= 0x7f || t >= 0xe0) return I;

                if constexpr(sizeof(e_t) <= 2)
                {
                                                     if(t == msgpack_posi8 ::tag || t == msgpack_negi8 ::tag) return I;
                    if constexpr(sizeof(e_t) == 2) { if(t == msgpack_posi16::tag || t == msgpack_negi16::tag) return I; }
                }
                else
                {
                    if(msgpack_posi8::tag <= t && t <= msgpack_posi64::tag)
                    {
                        if constexpr(sizeof(e_t) == 4) { if(t != msgpack_posi64::tag && t != msgpack_negi64::tag) return I; }
                        else                                                                                      return I;
                    }
                }
            }
            else if constexpr(_str_<e_t>)
            {
                if((t & 0xe0) == 0xa0 || (msgpack_str8::tag <= t && t <= msgpack_str32::tag)) return I;
            }
            else if constexpr(_jobj_<e_t>)
            {
                if((t & 0xf0) == 0x80 || t == msgpack_map16::tag || t == msgpack_map32::tag) return I;
            }
            else if constexpr(_tuple_like_<e_t> || std::ranges::range<e_t>)
            {
                if((t & 0xf0) == 0x90 || t == msgpack_arr16::tag || t == msgpack_arr32::tag) return I;
            }
            else
            {
                static_assert(false, "unsupported type");
            }

            return msgpack_errc::tag_mismatch;
        },
        [](auto& r)
        {
            return has_val(r);
        }
    );
}


template<unsigned Opts = 0>
class msgpack_reader_t
{
public:
    using is_jreader = void;

    static constexpr size_t padding = 0;
    static constexpr size_t default_max_doc_size = -1; // has no effect

    // returns offset after last read char.
    static constexpr expected<size_t> read(_byte_buf_ auto const& b, auto&& f, size_t offset = 0)
    {
        auto v = msgpack_value<Opts>(b, offset);

        DSK_E_TRY_ONLY(f(v));

        return static_cast<size_t>(v.cur_pos() - buf_data<uint8_t>(b));
    }
};

using msgpack_reader = msgpack_reader_t<>;


// returns offset after last read char.
template<unsigned Opts = 0>
constexpr expected<size_t> read_msgpack(auto&& def, _byte_buf_ auto const& b, auto& v, size_t offset = 0)
{
    return jread(def, msgpack_reader_t<Opts>(), b, v, offset);
}

template<class T, unsigned Opts = 0>
constexpr expected<size_t> read_msgpack_foreach(auto&& def, _byte_buf_ auto const& b, auto&& f, size_t offset = 0)
{
    return jread_foreach<T>(def, msgpack_reader_t<Opts>(), b, DSK_FORWARD(f), offset);
}

template<unsigned Opts = 0>
constexpr expected<size_t> read_msgpack_range(auto&& def, _byte_buf_ auto const& b, auto& vs, size_t offset = 0)
{
    return jread_range(def, msgpack_reader_t<Opts>(), b, vs, offset);
}

template<unsigned Opts = 0>
constexpr expected<size_t> read_msgpack_range(auto&& def, _byte_buf_ auto const& b, size_t maxV, auto& vs, size_t offset = 0)
{
    return jread_range_at_most(def, msgpack_reader_t<Opts>(), b, maxV, vs, offset);
}


template<unsigned Opts = 0>
constexpr error_code read_msgpack_whole(auto&& def, _byte_buf_ auto const& b, auto& v, size_t offset = 0)
{
    return jread_whole(def, msgpack_reader_t<Opts>(), b, v, offset);
}

template<unsigned Opts = 0>
constexpr error_code read_msgpack_range_whole(auto&& def, _byte_buf_ auto const& b, auto& vs, size_t offset = 0)
{
    return jread_range_whole(def, msgpack_reader_t<Opts>(), b, vs, offset);
}


} // namespace dsk
