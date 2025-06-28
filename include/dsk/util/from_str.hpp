#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/str.hpp>
#include <charconv>


namespace dsk{


template<bool WholeStr = false>
constexpr auto _from_str(_byte_str_ auto const& s, _arithmetic_ auto& v, auto baseOrFmt) noexcept
->
    std::conditional_t<WholeStr, error_code, expected<size_t/*decltype(str_data(s))*/>>
{
    auto* beg = str_begin<char>(s);
    auto* end = str_end<char>(s);

    auto[p, ec] = std::from_chars(beg, end, v, baseOrFmt);

    if(has_err(ec))
    {
        return make_error_code(ec);
    }

    if constexpr(WholeStr)
    {
        if(p != end) return errc::input_not_fully_consumed;
        else         return {};
    }
    else
    {
        //return reinterpret_cast<decltype(str_data(s))>(p);
        return static_cast<size_t>(p - beg);
    }
}


constexpr expected<size_t> from_str(_byte_str_ auto const& s, std::integral auto& v, int base = 10) noexcept
{    
    return _from_str(s, v, base);
}

constexpr expected<size_t> from_str(_byte_str_ auto const& s, std::floating_point auto& v,
                                    std::chars_format fmt = std::chars_format::general) noexcept
{
    return _from_str(s, v, fmt);
}


constexpr error_code from_whole_str(_byte_str_ auto const& s, std::integral auto& v, int base = 10) noexcept
{
    return _from_str<true>(s, v, base);
}

constexpr error_code from_whole_str(_byte_str_ auto const& s, std::floating_point auto& v,
                                    std::chars_format fmt = std::chars_format::general) noexcept
{
    return _from_str<true>(s, v, fmt);
}


template<std::integral T>
constexpr expected<T> str_to(_byte_str_ auto const& s, int base = 10) noexcept
{
    T v;
    DSK_E_TRY_ONLY(from_whole_str(s, v, base));
    return v;
}

template<std::floating_point T>
constexpr expected<T> str_to(_byte_str_ auto const& s) noexcept
{
    T v;
    DSK_E_TRY_ONLY(from_whole_str(s, v));
    return v;
}


} // namespace dsk
