#pragma once

#include <dsk/config.hpp>
#include <dsk/util/ct_vector.hpp>
#include <dsk/util/type_name.hpp>
#include <array>
#include <tuple>
#include <ranges>
#include <utility>


namespace dsk{


#if __has_builtin(__type_pack_elment)

template<size_t I, class... T>
using type_pack_elm = __type_pack_elment<I, T...>;

#else

namespace type_pack_elm_detail
{
    template<class T, size_t I> struct elm          {};
    template<class... Elm     > struct pack : Elm...{};

    template<size_t I, class T> auto get_base_elm(elm<T, I>) -> T;

    template<size_t I, class... T, size_t... J>
    auto get_elm(std::index_sequence<J...>) -> decltype(get_base_elm<I>(pack<elm<T, J>...>()));
}  // namespace type_pack_elm_detail

template<size_t I, class... T>
using type_pack_elm = decltype(type_pack_elm_detail::get_elm<I, T...>(std::make_index_sequence<sizeof...(T)>()));

#endif



template<size_t I, template<class...> class L, class... T>
auto type_list_elm_impl(L<T...> const&) -> type_pack_elm<I, T...>;

template<size_t I, class L>
using type_list_elm = decltype(type_list_elm_impl<I>(std::declval<L>()));


template<size_t I, auto... V>
inline constexpr auto value_pack_elm = type_pack_elm<I, constant<V>...>::value;


} // namespace dsk
