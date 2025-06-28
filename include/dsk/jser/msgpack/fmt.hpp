#pragma once

#include <dsk/config.hpp>
#include <dsk/util/concepts.hpp>


namespace dsk{


template<uint8_t Tag, class Type>
struct msgpack_tagged_fmt
{
    static constexpr uint8_t tag = Tag;
    using type = Type;
};

// positive integer requiring N bits, not matter the type is signed or not.
using msgpack_posi8  = msgpack_tagged_fmt<0xcc, uint8_t >;
using msgpack_posi16 = msgpack_tagged_fmt<0xcd, uint16_t>;
using msgpack_posi32 = msgpack_tagged_fmt<0xce, uint32_t>;
using msgpack_posi64 = msgpack_tagged_fmt<0xcf, uint64_t>;

// negative integer requiring N bits, signed type only.
using msgpack_negi8  = msgpack_tagged_fmt<0xd0, int8_t >;
using msgpack_negi16 = msgpack_tagged_fmt<0xd1, int16_t>;
using msgpack_negi32 = msgpack_tagged_fmt<0xd2, int32_t>;
using msgpack_negi64 = msgpack_tagged_fmt<0xd3, int64_t>;

using msgpack_float  = msgpack_tagged_fmt<0xca, float >;
using msgpack_double = msgpack_tagged_fmt<0xcb, double>;

using msgpack_arr16 = msgpack_tagged_fmt<0xdc, uint16_t>;
using msgpack_arr32 = msgpack_tagged_fmt<0xdd, uint32_t>;
using msgpack_map16 = msgpack_tagged_fmt<0xde, uint16_t>;
using msgpack_map32 = msgpack_tagged_fmt<0xdf, uint32_t>;

using msgpack_str8  = msgpack_tagged_fmt<0xd9, uint8_t >;
using msgpack_str16 = msgpack_tagged_fmt<0xda, uint16_t>;
using msgpack_str32 = msgpack_tagged_fmt<0xdb, uint32_t>;

using msgpack_bin8  = msgpack_tagged_fmt<0xc4, uint8_t >;
using msgpack_bin16 = msgpack_tagged_fmt<0xc5, uint16_t>;
using msgpack_bin32 = msgpack_tagged_fmt<0xc6, uint32_t>;

using msgpack_ext8  = msgpack_tagged_fmt<0xc7, uint8_t >;
using msgpack_ext16 = msgpack_tagged_fmt<0xc8, uint16_t>;
using msgpack_ext32 = msgpack_tagged_fmt<0xc9, uint32_t>;

inline constexpr uint8_t msgpack_tag_fixext1  = 0xd4;
inline constexpr uint8_t msgpack_tag_fixext2  = 0xd5;
inline constexpr uint8_t msgpack_tag_fixext4  = 0xd6;
inline constexpr uint8_t msgpack_tag_fixext8  = 0xd7;
inline constexpr uint8_t msgpack_tag_fixext16 = 0xd8;

inline constexpr uint8_t msgpack_nil   = 0xc0;
inline constexpr uint8_t msgpack_false = 0xc2;
inline constexpr uint8_t msgpack_true  = 0xc3;


// NOTE: Tag < 0 is reserved for official use.
template<int8_t Tag, class Type>
struct msgpack_ext_fmt
{
    static constexpr int8_t tag = Tag;
    using type = Type;
};

using msgpack_packed_arr_uint8  = msgpack_ext_fmt<60, uint8_t >;
using msgpack_packed_arr_uint16 = msgpack_ext_fmt<61, uint16_t>;
using msgpack_packed_arr_uint32 = msgpack_ext_fmt<62, uint32_t>;
using msgpack_packed_arr_uint64 = msgpack_ext_fmt<63, uint64_t>;

using msgpack_packed_arr_int8  = msgpack_ext_fmt<64, int8_t >;
using msgpack_packed_arr_int16 = msgpack_ext_fmt<65, int16_t>;
using msgpack_packed_arr_int32 = msgpack_ext_fmt<66, int32_t>;
using msgpack_packed_arr_int64 = msgpack_ext_fmt<67, int64_t>;

using msgpack_packed_arr_float  = msgpack_ext_fmt<68, float >;
using msgpack_packed_arr_double = msgpack_ext_fmt<69, double>;


template<class T>
using msgpack_packed_arr_of = typename decltype([]()
{
         if constexpr(_same_as_<T, uint8_t >) return type_c<msgpack_packed_arr_uint8 >;
    else if constexpr(_same_as_<T, uint16_t>) return type_c<msgpack_packed_arr_uint16>;
    else if constexpr(_same_as_<T, uint32_t>) return type_c<msgpack_packed_arr_uint32>;
    else if constexpr(_same_as_<T, uint64_t>) return type_c<msgpack_packed_arr_uint64>;

    else if constexpr(_same_as_<T, int8_t >) return type_c<msgpack_packed_arr_int8 >;
    else if constexpr(_same_as_<T, int16_t>) return type_c<msgpack_packed_arr_int16>;
    else if constexpr(_same_as_<T, int32_t>) return type_c<msgpack_packed_arr_int32>;
    else if constexpr(_same_as_<T, int64_t>) return type_c<msgpack_packed_arr_int64>;

    else if constexpr(_same_as_<T, float >) return type_c<msgpack_packed_arr_float >;
    else if constexpr(_same_as_<T, double>) return type_c<msgpack_packed_arr_double>;
    else
        static_assert(false, "unsupported type");
}())::type;


enum msgpack_opt_e : unsigned
{
    msgpack_opt_little_endian = 1,
    msgpack_opt_packed_arr = 1 << 1,

    msgpack_opt_le_parr = msgpack_opt_little_endian | msgpack_opt_packed_arr,
};


} // namespace dsk
