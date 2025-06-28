#pragma once

#include <dsk/config.hpp>
#include <boost/endian/conversion.hpp>


namespace dsk{


namespace bendian = boost::endian;

using endian_order = bendian::order;


// NOTE:
// sizeof(T) must be 1, 2, 4, or 8
// 1 <= N <= sizeof(T)
// T is TriviallyCopyable
// if N < sizeof(T), T must be integral or enum

template<endian_order Order, size_t N = 0, class T>
void store_xe(void* p, T v) noexcept
{
    if constexpr(std::is_same_v<T, bool>)
    {
        reinterpret_cast<char*>(p)[0] = (v ? 'T' : 'F');
    }
    else
    {
        bendian::endian_store<T, N ? N : sizeof(T), Order>(reinterpret_cast<unsigned char*>(p), v);
    }
}

template<endian_order Order, class T, size_t N = 0>
T load_xe(void const* p) noexcept
{
    if constexpr(std::is_same_v<T, bool>)
    {
        return reinterpret_cast<char const*>(p)[0] == 'T';
    }
    else
    {
        return bendian::endian_load<T, N ? N : sizeof(T), Order>(reinterpret_cast<unsigned char const*>(p));
    }
}

template<endian_order Order, size_t N = 0, class T>
void load_xe(void const* p, T& v) noexcept
{
    v = load_xe<Order, T, N>(p);
}

template<size_t N = 0> void store_le(void* p, auto v) noexcept { store_xe<endian_order::little, N>(p, v); }
template<size_t N = 0> void store_be(void* p, auto v) noexcept { store_xe<endian_order::big   , N>(p, v); }

template<class T, size_t N = 0> T load_le(void const* p) noexcept { return load_xe<endian_order::little, T, N>(p); }
template<class T, size_t N = 0> T load_be(void const* p) noexcept { return load_xe<endian_order::big   , T, N>(p); }

template<size_t N = 0> void load_le(void const* p, auto& v) noexcept { load_xe<endian_order::little, N>(p, v); }
template<size_t N = 0> void load_be(void const* p, auto& v) noexcept { load_xe<endian_order::big   , N>(p, v); }


} // namespace dsk
