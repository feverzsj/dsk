#pragma once

#include <dsk/config.hpp>
#include <dsk/util/ct_basic.hpp>
#include <dsk/util/index_array.hpp>
#include <dsk/util/type_traits.hpp>
#include <variant>


namespace dsk{


template<class T>
concept _variant_ = requires{ std::variant_size<std::remove_cvref_t<T>>::value; }; // variant_size_v won't work here


template<auto Indices>
constexpr decltype(auto) visit_var(auto&& var, auto&& f)
{
    return visit_ct_elm<Indices>(var.index(), [&]<size_t I>() -> decltype(auto)
    {
        return may_invoke_with_nts<I>(DSK_FORWARD(f), std::get<I>(DSK_FORWARD(var)));
    });
}

constexpr decltype(auto) visit_var(auto&& var, auto&& f)
{
    return visit_var
    <
        make_index_array<std::variant_size_v<DSK_NO_CVREF_T(var)>>()
    >
    (DSK_FORWARD(var), DSK_FORWARD(f));
}


constexpr decltype(auto) assure_var_holds(auto& var, size_t i, auto&&... f)
{
    static_assert(sizeof...(f) <= 1);

    constexpr size_t varSize = std::variant_size_v<DSK_NO_CVREF_T(var)>;

    DSK_ASSERT(0 <= i && i < varSize);

    if(var.index() != i)
    {
        return visit_ct_elm<make_index_array<varSize>()>(i, [&]<size_t I>() -> decltype(auto)
        {
            auto& v = var.template emplace<I>();

            if constexpr(sizeof...(f))
            {
                return (..., may_invoke_with_nts<I>(DSK_FORWARD(f), v));
            }
        });
    }
    else
    {
        if constexpr(sizeof...(f))
        {
            return visit_var(var, DSK_FORWARD(f)...);
        }
    }
}

template<size_t I>
constexpr decltype(auto) assure_var_holds(auto& var, auto&&... f)
{
    static_assert(sizeof...(f) <= 1);

    if(var.index() != I)
    {
        auto& v = var.template emplace<I>();

        if constexpr(sizeof...(f))
        {
            return (..., may_invoke_with_nts<I>(DSK_FORWARD(f), v));
        }
    }
    else
    {
        if constexpr(sizeof...(f))
        {
            return (..., may_invoke_with_nts<I>(DSK_FORWARD(f), std::get<I>(var)));
        }
    }
}



} // namespace dsk
