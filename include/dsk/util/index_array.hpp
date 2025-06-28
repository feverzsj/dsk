#pragma once

#include <dsk/config.hpp>
#include <array>
#include <algorithm>


namespace dsk{


template<size_t N>
using index_array = std::array<size_t, N>;


template<size_t Lower, size_t Upper>
constexpr auto make_index_array() noexcept
{
    static_assert(Lower <= Upper);

    if constexpr(Upper - Lower)
    {
        index_array<Upper - Lower> arr;

        for(size_t i = 0; i < arr.size(); ++i)
            arr[i] = Lower + i;

        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

template<size_t N>
constexpr auto make_index_array() noexcept
{
    return make_index_array<0, N>();
}

template<size_t N>
constexpr auto make_index_array(size_t v) noexcept
{
    if constexpr(N)
    {
        index_array<N> arr;
        std::ranges::fill(arr, v);
        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

template<auto Indices, size_t Beg, size_t End>
constexpr auto make_index_array() noexcept
{
    static_assert(Beg <= End && End <= std::size(Indices));

    if constexpr(End - Beg)
    {
        index_array<End - Beg> arr;
        std::ranges::copy(std::begin(Indices) + Beg, std::begin(Indices) + End, arr.data());
        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

constexpr auto make_index_array(auto... v) noexcept
{
    if constexpr(sizeof...(v))
    {
        return index_array<sizeof...(v)>{v...};
    }
    else
    {
        return empty_range<size_t>;
    }
}


template<size_t Offset, size_t Lower, size_t Upper>
constexpr auto make_index_array_offset() noexcept
{
    static_assert(Lower <= Upper);

    if constexpr(Offset + Upper - Lower)
    {
        index_array<Offset + Upper - Lower> arr;

        for(size_t i = 0; i < Offset; ++i)
            arr[i] = Lower;

        for(size_t i = Offset; i < arr.size(); ++i)
            arr[i] = Lower + i - Offset;

        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

template<size_t Offset, size_t N>
constexpr auto make_index_array_offset() noexcept
{
    return make_index_array_offset<Offset, 0, N>();
}


template<size_t Lower, size_t Upper>
constexpr auto make_even_index_array() noexcept
{
    static_assert(Lower <= Upper);
    static_assert((Upper - Lower) % 2 == 0);

    if constexpr((Upper - Lower) / 2)
    {
        index_array<(Upper - Lower) / 2> arr;

        for(size_t i = 0; i < arr.size(); ++i)
            arr[i] = (Lower + i)*2;

        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

// NOTE: it's index based, so it's {0, 2, 4, 6, ...}
template<size_t N>
constexpr auto make_even_index_array() noexcept
{
    return make_even_index_array<0, N>();
}


template<size_t Lower, size_t Upper>
constexpr auto make_odd_index_array() noexcept
{
    static_assert(Lower <= Upper);
    static_assert((Upper - Lower) % 2 == 0);

    if constexpr((Upper - Lower) / 2)
    {
        index_array<(Upper - Lower) / 2> arr;

        for(size_t i = 0; i < arr.size(); ++i)
            arr[i] = (Lower + i)*2 + 1;

        return arr;
    }
    else
    {
        return empty_range<size_t>;
    }
}

template<size_t N>
constexpr auto make_odd_index_array() noexcept
{
    return make_odd_index_array<0, N>();
}


} // namespace dsk
