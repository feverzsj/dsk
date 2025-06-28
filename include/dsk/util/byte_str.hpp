#pragma once

#include <dsk/util/str.hpp>


namespace dsk
{

inline constexpr struct to_byte_str_cpo
{
    constexpr decltype(auto) operator()(auto&& t) const
    requires(
        _byte_str_<decltype(t)>
        || requires{ dsk_to_byte_str(*this, const_cast<DSK_NO_CVREF_T(t)&>(t)); })
    {
        if constexpr(_byte_str_<decltype(t)>)
        {
            return DSK_FORWARD(t);
        }
        else
        {
            return dsk_to_byte_str(*this, const_cast<DSK_NO_CVREF_T(t)&>(t));
        }
    }
} to_byte_str;


} // namespace dsk
