#pragma once

#include <dsk/config.hpp>
#include <dsk/util/str.hpp>
#include <functional>


namespace dsk{


// heterogeneous comparator
template<class T, auto Comp>
struct heterogeneous_comparator;

template<class T>
concept _heterogeneous_ = requires{ heterogeneous_comparator<T, null_op>(); };


template<_handle_ T, auto Comp>
struct heterogeneous_comparator<T, Comp>
{
    using is_transparent = void;

    constexpr bool operator()(T const& l, T const& r) const noexcept
    {
        return Comp(l, r);
    }

    constexpr bool operator()(T const& l, _similar_handle_to_<T> auto const& r) noexcept
    {
        return Comp(get_handle(l), get_handle(r));
    }

    constexpr bool operator()(_similar_handle_to_<T> auto const& r, T const& l) noexcept
    {
        return Comp(get_handle(l), get_handle(r));
    }
};

template<_str_ T, auto Comp>
struct heterogeneous_comparator<T, Comp>
{
    using is_transparent = void;

    constexpr bool operator()(T const& l, T const& r) const noexcept
    {
        return Comp(l, r);
    }

    constexpr bool operator()(T const& l, _similar_str_<T> auto const& r) noexcept
    {
        return Comp(str_view(l), str_view(r));
    }

    constexpr bool operator()(_similar_str_<T> auto const& r, T const& l) noexcept
    {
        return Comp(str_view(l), str_view(r));
    }
};

template<class T>
using less = DSK_CONDITIONAL_T(
                _heterogeneous_<T>,
                    DSK_PARAM(heterogeneous_comparator<T, std::less<>{}>),
                    std::less<>);

template<class T>
using equal_to = DSK_CONDITIONAL_T(
                    _heterogeneous_<T>,
                        DSK_PARAM(heterogeneous_comparator<T, std::equal_to<>{}>),
                        std::equal_to<>);


} // namespace dsk
