#pragma once

#include <dsk/config.hpp>
#include <dsk/util/type_traits.hpp>


namespace dsk{


template<bool Enable = true>
[[nodiscard]] constexpr auto scope_exit(auto&& f)
{
    if constexpr(Enable)
    {
        struct scope_exit_t
        {
            DSK_NO_CVREF_T(f) f;
            ~scope_exit_t(){ f(); }
        };

        return scope_exit_t{DSK_FORWARD(f)};
    }
    else
    {
        return null_op;
    }
}

template<bool Enable = true>
[[nodiscard]] constexpr auto scope_exit_if(bool cond, auto&& f)
{
    if constexpr(Enable)
    {
        struct scope_exit_t
        {
            bool cond;
            DSK_NO_CVREF_T(f) f;
            ~scope_exit_t(){ if(cond) f(); }
        };

        return scope_exit_t{cond, DSK_FORWARD(f)};
    }
    else
    {
        return null_op;
    }
}


} // namespace dsk
