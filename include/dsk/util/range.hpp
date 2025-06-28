#pragma once

#include <dsk/config.hpp>
#include <ranges>
#include <utility>
#include <iterator>
#include <algorithm>


namespace dsk{


constexpr auto single_value_range(auto& v) noexcept
{
    return std::ranges::subrange(&v, 1);
}

constexpr auto move_range(std::input_or_output_iterator auto first, std::sentinel_for<decltype(first)> auto last) noexcept
{
    if constexpr(std::is_lvalue_reference_v<decltype(*first)>)
    {
        return std::ranges::subrange(std::move_iterator(first), std::move_sentinel(last));
    }
    else
    {
        return std::ranges::subrange(first, last);
    }
}

constexpr auto move_range(std::ranges::borrowed_range auto&& r) noexcept
{
    return move_range(std::ranges::begin(r), std::ranges::end(r));
}

constexpr auto reverse_range(std::ranges::borrowed_range auto&& r) noexcept
{
    return std::ranges::subrange(std::ranges::rbegin(r), std::ranges::rend(r));
}


} // namespace dsk
