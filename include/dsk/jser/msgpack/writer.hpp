#pragma once

#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/jser/cpo.hpp>
#include <dsk/jser/msgpack/fmt.hpp>


namespace dsk{


template<_resizable_byte_buf_ Buf, unsigned Opts>
class msgpack_writer
{
    using char_type = buf_val_t<Buf>;

    Buf& _b;

    static constexpr bool useLittleEndian = Opts & msgpack_opt_little_endian;
    static constexpr bool usePackedArr = Opts & msgpack_opt_packed_arr;
    static constexpr auto endian = useLittleEndian ? endian_order::little : endian_order::big;

    constexpr void write_lsb(std::integral auto v)
    {
        buy_buf(_b, 1)[0] = static_cast<char_type>(v);
    }

    template<class Fmt>
    constexpr void write(_arithmetic_ auto v)
    {
        using type = typename Fmt::type;

        constexpr size_t n = 1 + sizeof(type);

        auto* d = buy_buf(_b, n);

        d[0] = static_cast<char_type>(Fmt::tag);
        
        store_xe<endian>(d + 1, static_cast<type>(v));
    }

    void write(char_type const* h, size_t n, _byte_buf_ auto const& b)
    {
        append_buf(_b, std::span(h, n), b);
    }

    template<class Fmt>
    static size_t write(char_type* h, size_t n) noexcept
    {
        h[0] = static_cast<char_type>(Fmt::tag);
        store_xe<endian>(h + 1, static_cast<typename Fmt::type>(n));
        return 1 + sizeof(typename Fmt::type);
    }

public:
    constexpr explicit msgpack_writer(Buf& b) noexcept
        : _b(b)
    {}

    // NOTE: msgpack encodes positive integer as msgpack_uintXX which requires smallest number of bytes
    //                   and negative integer as msgpack_intXX which requires smallest number of bytes.

    constexpr void uint8(_unsigned_ auto v)
    {
        if(v < (1<<7)) write_lsb(v);
        else           write<msgpack_posi8>(v);
    }

    constexpr void uint16(_unsigned_ auto v)
    {
             if(v < (1<<7)) write_lsb(v);
        else if(v < (1<<8)) write<msgpack_posi8 >(v);
        else                write<msgpack_posi16>(v);
    }

    constexpr void uint32(_unsigned_ auto v)
    {
        if(v < (1<<8))
        {
            uint8(v);
        }
        else
        {
            if(v < (1<<16)) write<msgpack_posi16>(v);
            else            write<msgpack_posi32>(v);
        }
    }

    constexpr void uint64(_unsigned_ auto v)
    {
        if(v < (1<<8))
        {
            uint8(v);
        }
        else
        {
                 if(v < (1<<16)   ) write<msgpack_posi16>(v);
            else if(v < (1ULL<<32)) write<msgpack_posi32>(v);
            else                    write<msgpack_posi64>(v);
        }
    }


    constexpr void int8(auto v)
    {
        if(v < -(1<<5)) write<msgpack_negi8>(v);
        else            write_lsb(v);
    }

    constexpr void int16(auto v)
    {
        if(v < -(1<<5))
        {
            if(v < -(1<<7)) write<msgpack_negi16>(v);
            else            write<msgpack_negi8 >(v);
        }
        else if(v < (1<<7))
        {
            write_lsb(v);
        }
        else
        {
            if(v < (1<<8)) write<msgpack_posi8 >(v);
            else           write<msgpack_posi16>(v);
        }
    }

    constexpr void int32(auto v)
    {
        if(v < -(1<<5))
        {
                 if(v < -(1<<15)) write<msgpack_negi32>(v);
            else if(v < -(1<<7) ) write<msgpack_negi16>(v);
            else                  write<msgpack_negi8 >(v);
        }
        else if(v < (1<<7))
        {
            write_lsb(v);
        }
        else
        {
                 if(v < (1<<8) ) write<msgpack_posi8 >(v);
            else if(v < (1<<16)) write<msgpack_posi16>(v);
            else                 write<msgpack_posi32>(v);
        }
    }

    constexpr void int64(auto v)
    {
        if(v < -(1<<5))
        {
            if(v < -(1<<15))
            {
                if(v < -(1LL<<31)) write<msgpack_negi64>(v);
                else               write<msgpack_negi32>(v);
            }
            else
            {
                if(v < -(1<<7)) write<msgpack_negi16>(v);
                else            write<msgpack_negi8 >(v);
            }
        }
        else if(v < (1<<7))
        {
            write_lsb(v);
        }
        else
        {
            if(v < (1<<16))
            {
                if(v < (1<<8)) write<msgpack_posi8 >(v);
                else           write<msgpack_posi16>(v);
            }
            else
            {
                if(v < (1LL<<32)) write<msgpack_posi32>(v);
                else              write<msgpack_posi64>(v);
            }
        }
    }

    constexpr void float_(float v)
    {
        //if(v == v) // check for nan
        //{
        //    // compare v to limits to avoid undefined behavior
        //    if(v >= 0 && v <= float(std::numeric_limits<uint64_t>::max()) && v == float(uint64_t(v)))
        //    {
        //        uint64(uint64_t(v));
        //        return;
        //    }
        //
        //    if(v < 0 && v >= float(std::numeric_limits<int64_t>::min()) && v == float(int64_t(v)))
        //    {
        //        int64(int64_t(v));
        //        return;
        //    }
        //}

        write<msgpack_float>(v);
    }

    constexpr void double_(double v)
    {
        //if(v == v) // check for nan
        //{
        //    // compare v to limits to avoid undefined behavior
        //    if(v >= 0 && v <= double(std::numeric_limits<uint64_t>::max()) && v == double(uint64_t(v)))
        //    {
        //        uint64(uint64_t(v));
        //        return;
        //    }
        //
        //    if(v < 0 && v >= double(std::numeric_limits<int64_t>::min()) && v == double(int64_t(v)))
        //    {
        //        int64(int64_t(v));
        //        return;
        //    }
        //}

        write<msgpack_double>(v);
    }

    constexpr void nil()
    {
        write_lsb(msgpack_nil);
    }

    constexpr void bool_(bool v)
    {
        write_lsb(v ? msgpack_true : msgpack_false);
    }

    constexpr void str(_byte_str_ auto const& s)
    {
        auto b = str_range(s);
        auto n = buf_size(b);

        char_type h[5];
        size_t hn = [&]()
        {
            if(n < 32)
            {
                h[0] = static_cast<char_type>(0xa0 | n);
                return size_t(1);
            }

            if(n <   256) return write<msgpack_str8 >(h, n);
            if(n < 65536) return write<msgpack_str16>(h, n);
            else          return write<msgpack_str32>(h, n);
        }();

        write(h, hn, b);
    }

    constexpr void bin(auto const& v)
    {
        auto&& b = as_buf_of<char_type>(v);
        auto   n = buf_size(b);

        char_type h[5];
        size_t hn = [&]()
        {
            if(n <   256) return write<msgpack_bin8 >(h, n);
            if(n < 65536) return write<msgpack_bin16>(h, n);
            else          return write<msgpack_bin32>(h, n);
        }();

        write(h, hn, b);
    }

    constexpr void begin_arr(uint32_t n)
    {
             if(n <    16) write_lsb(0x90 | n);
        else if(n < 65536) write<msgpack_arr16>(n);
        else               write<msgpack_arr32>(n);
    }

    constexpr void begin_map(uint32_t n)
    {
             if(n <    16) write_lsb(0x80 | n);
        else if(n < 65536) write<msgpack_map16>(n);
        else               write<msgpack_map32>(n);
    }

    constexpr void fixext(int8_t tag, _pod_ auto const& v)
    {
        char_type h[2] =
        {
            []()
            {
                     if constexpr(sizeof(v) == 1 ) return msgpack_tag_fixext1 ;
                else if constexpr(sizeof(v) == 2 ) return msgpack_tag_fixext2 ;
                else if constexpr(sizeof(v) == 4 ) return msgpack_tag_fixext4 ;
                else if constexpr(sizeof(v) == 8 ) return msgpack_tag_fixext8 ;
                else if constexpr(sizeof(v) == 16) return msgpack_tag_fixext16;
                else
                     static_assert(false, "invalid size");
            }(),
            static_cast<char_type>(tag)
        };

        append_buf(_b, h, as_buf_of<char_type>(v));
    }

    constexpr void ext(int8_t tag, auto const& v)
    {
        auto&& b = as_buf_of<char_type>(v);
        auto   n = buf_size(b);

        char_type h[6];
        size_t hn = [&]()
        {
            if(n <   256) return write<msgpack_ext8 >(h, n);
            if(n < 65536) return write<msgpack_ext16>(h, n);
            else          return write<msgpack_ext32>(h, n);
        }();

        h[hn++] = static_cast<char_type>(tag);

        write(h, hn, b);
    }

    template<class Fmt>
    constexpr void ext(auto const& v)
    {
        ext(Fmt::tag, v);
    }

    [[nodiscard]] constexpr char_type* begin_ext(uint32_t nBytes, int8_t tag)
    {
        size_t os = buf_size(_b);
        auto* h = buy_buf(_b, 6 + nBytes);

        size_t hn = [&]()
        {
            if(nBytes <   256) return write<msgpack_ext8 >(h, nBytes);
            if(nBytes < 65536) return write<msgpack_ext16>(h, nBytes);
            else               return write<msgpack_ext32>(h, nBytes);
        }();

        h[hn++] = static_cast<char_type>(tag);

        resize_buf(_b, os + hn + nBytes);

        return h + hn;
    }

    template<class Fmt>
    [[nodiscard]] constexpr char_type* begin_ext(uint32_t nBytes)
    {
        return begin_ext(nBytes, Fmt::tag);
    }

    using is_jwriter = void;

    constexpr void null() { nil(); }

    template<_arithmetic_ T>
    constexpr void arithmetic(T v) noexcept
    {
             if constexpr(_same_as_<T,   bool>)   bool_(v);
        else if constexpr(_same_as_<T,  float>)  float_(v);
        else if constexpr(_same_as_<T, double>) double_(v);
        else if constexpr(_unsigned_<T>)
        {
                 if constexpr(sizeof(T) == 1)  uint8(v);
            else if constexpr(sizeof(T) == 2) uint16(v);
            else if constexpr(sizeof(T) == 4) uint32(v);
            else if constexpr(sizeof(T) == 8) uint64(v);
            else
                static_assert(false, "unsupported type");
        }
        else if constexpr(_signed_integral_<T>)
        {
                 if constexpr(sizeof(T) == 1)  int8(v);
            else if constexpr(sizeof(T) == 2) int16(v);
            else if constexpr(sizeof(T) == 4) int32(v);
            else if constexpr(sizeof(T) == 8) int64(v);
            else
                static_assert(false, "unsupported type");
        }
        else
            static_assert(false, "unsupported type");
    }

    constexpr void str_noescape(_byte_str_ auto const& v)
    {
        str(v);
    }

    constexpr void binary(auto const& v)
    {
        bin(v);
    }


    constexpr void arr_scope(uint32_t n, auto&& f)
    {
        begin_arr(n);
        f();
    }

    constexpr void elm_scope(auto&& ef)
    {
        ef();
    }

    constexpr void obj_scope(size_t maxCnt, auto&& f)
    {
        auto os = buf_size(_b);

        buy_buf(_b, (maxCnt <    16 ? 1 :(
                     maxCnt < 65536 ? 3 :
                                      5)));

        size_t n = f();

        DSK_ASSERT(n <= maxCnt);

        auto* d = buf_data(_b) + os;

             if(maxCnt <    16) *d = static_cast<char_type>(0x80 | n);
        else if(maxCnt < 65536) write<msgpack_map16>(d, n);
        else                    write<msgpack_map32>(d, n);
    }

    constexpr void fld_scope(auto const& k, auto&& vf)
    {
        auto&& q = jkey_uint_or_as_is(k);

             if constexpr(       _str_<decltype(q)>) str(q);
        else if constexpr(_arithmetic_<decltype(q)>) arithmetic(q);
        else
            static_assert(false, "unsupported key type");

        vf();
    }


    constexpr void packed_arr_buf(_buf_ auto const& v) requires(usePackedArr && endian == endian_order::native)
    {
        ext<msgpack_packed_arr_of<buf_val_t<decltype(v)>>>(v);
    }

    template<class T>
    struct msgpack_packed_arr_writer
    {
        char_type* _d;

        using is_jwriter = void;

        constexpr void arithmetic(auto v) noexcept
        {
            store_xe<endian>(_d, static_cast<T>(v));
            _d += sizeof(T);
        }

        constexpr void arr_scope(uint32_t, auto&& f) { f(); }
        constexpr void elm_scope(auto&& ef) { ef(); }
    };

    template<class T>
    constexpr void packed_arr_scope(uint32_t n, auto&& f) requires(usePackedArr)
    {
        f(msgpack_packed_arr_writer<T>(begin_ext<msgpack_packed_arr_of<T>>(n * sizeof(T))));
    }
};


template<unsigned Opts = 0>
constexpr auto make_msgpack_writer(_resizable_byte_buf_ auto& b) noexcept
{
    return msgpack_writer<DSK_NO_REF_T(b), Opts>(b);
}


template<unsigned Opts = 0>
constexpr void write_msgpack(auto&& def, auto const& v, _resizable_byte_buf_ auto& b)
{
    jwrite(def, v, make_msgpack_writer<Opts>(b));
}

template<unsigned Opts = 0>
constexpr void write_msgpack_range(auto&& def, std::ranges::range auto const& vs, _resizable_byte_buf_ auto& b)
{
    jwrite_range(def, vs, make_msgpack_writer<Opts>(b));
}


} // namespace dsk
