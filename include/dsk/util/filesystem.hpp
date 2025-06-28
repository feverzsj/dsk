#pragma once

#include <dsk/util/byte_str.hpp>
#include <dsk/default_allocator.hpp>
#include <filesystem>


namespace dsk{

                                                                             //v: use non const lref to avoid implicit conversion.
constexpr decltype(auto) dsk_to_byte_str(to_byte_str_cpo, std::filesystem::path& p)
{
    if constexpr(_byte_str_<decltype(p.native())>)
    {
        return p.native();
    }
    else
    {
        return p.string<char8_t>(DSK_DEFAULT_ALLOCATOR<char8_t>());
    }
}


template<class T>
constexpr decltype(auto) to_str_of(_no_cvref_same_as_<std::filesystem::path> auto&& p)
{
    if constexpr(_similar_str_of_<decltype(p.native()), T>)
    {
        return p.native();
    }
    else
    {
        return p.template string<T>(DSK_DEFAULT_ALLOCATOR<T>());
    }
}


} // namespace dsk
