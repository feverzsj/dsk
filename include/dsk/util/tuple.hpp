#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/ct_basic.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/ref_holder.hpp>
#include <dsk/util/index_array.hpp>
#include <dsk/util/type_pack_elm.hpp>
#include <tuple>
#include <array>
#include <ranges>
#include <utility>
#include <complex>


namespace dsk{


template<size_t I> using idx_t = constant<I>;
template<size_t I> inline constexpr auto idx_c = idx_t<I>();


// tuple<T&, Q&&...> is copyable;
// Unlike std::get(std::tuple&/&&), which simply returns T&/T&&,
// get_elm(tuple&/&&) returns T for _ref_<T>, T&/T&& for non reference.

template<size_t I, class T>
struct tuple_elm
{
    using type = T;

    DSK_NO_UNIQUE_ADDR std::conditional_t<_ref_<T>, ref_holder<T&&>, T> v;

    constexpr auto&& operator[](this auto&& self, idx_t<I>) noexcept
    {
        if constexpr(_ref_<T>) return to_similar_ref<tuple_elm>(DSK_FORWARD(self)).v.get();
        else                   return to_similar_ref<tuple_elm>(DSK_FORWARD(self)).v;
    }

    constexpr auto operator<=>(tuple_elm const& r) const noexcept requires(requires{ v <=> r.v; })
    {
        return v <=> r.v;
    }

    constexpr bool operator==(tuple_elm const& r) const noexcept requires(requires{ v == r.v; })
    {
        return v == r.v;
    }
};

template<class, class... T>
struct tuple_base;

template<size_t... I, class... T>
struct tuple_base<std::index_sequence<I...>, T...>
    : tuple_elm<I, T>...
{
    using tuple_elm<I, T>::operator[]...;

    constexpr auto operator<=>(tuple_base const&) const noexcept = default;

    static constexpr size_t size() noexcept { return sizeof...(T); }
    static constexpr size_t count = sizeof...(T);

    template<class E> auto& as_array()       noexcept { return alias_cast<E[count]>(*this); }
    template<class E> auto& as_array() const noexcept { return alias_cast<E[count]>(*this); }

    constexpr auto&& first(this auto&& self) noexcept { return DSK_FORWARD(self)[idx_c<0>]; }
    constexpr auto&& last (this auto&& self) noexcept { return DSK_FORWARD(self)[idx_c<count - 1>]; }

    // structured binding support
    template<size_t J> constexpr auto&& get(this auto&& self) noexcept { return DSK_FORWARD(self)[idx_c<J>]; }
};


template<class... T>
struct tuple : tuple_base<std::make_index_sequence<sizeof...(T)>, T...>
{
    using base = tuple_base<std::make_index_sequence<sizeof...(T)>, T...>;

    constexpr tuple() = default;

    constexpr explicit(sizeof...(T) == 1) tuple(auto&&... v)
    //noexcept(
    //    noexcept(base{{DSK_FORWARD(v)}...}))
    //requires(
    //    requires{ base{{DSK_FORWARD(v)}...}; })
        :
        base{{DSK_FORWARD(v)}...}
    {}

//     constexpr tuple(in_place_t, auto&&... v) requires(sizeof...(v))
//     noexcept(
//         noexcept(base{{DSK_FORWARD(v)}...})
//     )
//         : base{{DSK_FORWARD(v)}...}
//     {}
};

// template<>
// struct tuple<>
// {
//     constexpr auto operator<=>(tuple const&) const noexcept = default;
// };

template<class... T>
tuple(T...) -> tuple<T...>;

// overwrite implicit deduction guide which deduces tuple(tuple(0, 1)) to tuple<int, int>;
// we'd like our tuple(...) to act like make_tuple().
template<class... T>
tuple(tuple<T...>) -> tuple<tuple<T...>>;


static_assert(std::is_same_v<decltype(tuple(tuple())), tuple<tuple<>>>);
static_assert(std::is_same_v<decltype(tuple(tuple(0, 1.f))), tuple<tuple<int, float>>>);

static_assert(std::is_empty_v<tuple<>>);
static_assert(std::is_empty_v<tuple<tuple<>>>);
// static_assert(sizeof(tuple<int, tuple<>>) == sizeof(tuple<int>));
static_assert(sizeof(tuple<int, null_op_t, tuple<short>>) == sizeof(tuple<int, short>));
// static_assert(std::is_trivial_v<tuple<>>);
// static_assert(std::is_trivial_v<tuple<tuple<>>>);
// static_assert(std::is_trivial_v<tuple<char, int, float, unsigned, std::byte, tuple<>>>);
static_assert(std::is_trivially_move_constructible_v<tuple<>>);
static_assert(std::is_trivially_move_constructible_v<tuple<tuple<>>>);
static_assert(std::is_trivially_move_constructible_v<tuple<char, int, float, unsigned, std::byte, tuple<>>>);
static_assert(std::is_trivially_move_assignable_v<tuple<>>);
static_assert(std::is_trivially_move_assignable_v<tuple<tuple<>>>);
static_assert(std::is_trivially_move_assignable_v<tuple<char, int, float, unsigned, std::byte, tuple<>>>);
static_assert(std::is_nothrow_copy_constructible_v<tuple<>>);
static_assert(std::is_nothrow_copy_constructible_v<tuple<tuple<>>>);
static_assert(std::is_nothrow_copy_constructible_v<tuple<char, int, float, unsigned, std::byte, tuple<>>>);
static_assert(std::is_nothrow_move_constructible_v<tuple<>>);
static_assert(std::is_nothrow_move_constructible_v<tuple<tuple<>>>);
static_assert(std::is_nothrow_move_constructible_v<tuple<char, int, float, unsigned, std::byte, tuple<>>>);


template<class... T> void _tuple_impl(tuple<T...>&);
template<class T> concept _tuple_ = requires(std::remove_cvref_t<T>& t){ dsk::_tuple_impl(t); };


constexpr auto fwd_as_tuple(auto&&... a) noexcept
{
    return tuple<decltype(a)...>(DSK_FORWARD(a)...);
}

constexpr auto fwd_lref_decay_other_as_tuple(auto&&... a) noexcept
{
    return tuple<decay_non_lref_t<decltype(a)>...>(DSK_FORWARD(a)...);
}


template<bool Cond>
constexpr auto make_tuple_if(auto&&... a)
{
    if constexpr(Cond) return tuple(DSK_FORWARD(a)...);
    else               return tuple();
}

template<bool Cond>
constexpr auto fwd_as_tuple_if(auto&&... a) noexcept
{
    if constexpr(Cond) return fwd_as_tuple(DSK_FORWARD(a)...);
    else               return tuple();
}

template<bool Cond>
constexpr decltype(auto) fwd_tuple_if(auto&& t) noexcept
{
    if constexpr(Cond) return DSK_FORWARD(t);
    else               return tuple();
}



template<size_t I>
struct get_elm_cpo
{
    constexpr auto operator()(auto&& t) const noexcept
    ->decltype(
        dsk_get_elm(*this, DSK_FORWARD(t)))
    {
        static_assert(std::is_reference_v<decltype(dsk_get_elm(*this, DSK_FORWARD(t)))>);
        return dsk_get_elm(*this, DSK_FORWARD(t));
    }
};

template<size_t I>
inline constexpr auto get_elm = get_elm_cpo<I>();


template<size_t I>
constexpr auto&& dsk_get_elm(get_elm_cpo<I>, _tuple_ auto&& t) noexcept
{
    return DSK_FORWARD(t)[idx_c<I>];
}

template<size_t I>
constexpr auto&& dsk_get_elm(get_elm_cpo<I>, _array_ auto&& t) noexcept
{
    return DSK_FORWARD(t)[I];
}

template<size_t I>
constexpr auto&& dsk_get_elm(get_elm_cpo<I>, auto&& t) noexcept
requires(requires{
    {std::get<I>(DSK_FORWARD(t))} -> _ref_; })
{
    return std::get<I>(DSK_FORWARD(t));
}


struct elm_count_cpo{};

// T can have any cv-ref qualifier
template<class T>
inline constexpr size_t elm_count = decltype(dsk_elm_count(elm_count_cpo(), std::declval<std::remove_cvref_t<T>&>()))::value;

template<class... T        > auto dsk_elm_count(elm_count_cpo, tuple<T...>&      ) -> constant<sizeof...(T)>;
template<class T, size_t N > auto dsk_elm_count(elm_count_cpo, T(&)[N]           ) -> constant<N>;
template<class T           > auto dsk_elm_count(elm_count_cpo, T&                ) -> decltype(std::tuple_size<T>()); // won't match types derived from a std tuple like
template<class... T        > auto dsk_elm_count(elm_count_cpo, std::tuple<T...>& ) -> constant<sizeof...(T)>;
template<class T1, class T2> auto dsk_elm_count(elm_count_cpo, std::pair<T1, T2>&) -> constant<2>;
template<class T, size_t N > auto dsk_elm_count(elm_count_cpo, std::array<T, N>& ) -> constant<N>;
template<class T           > auto dsk_elm_count(elm_count_cpo, std::complex<T>&  ) -> constant<2>;
template<class It, class S, std::ranges::subrange_kind K>
auto dsk_elm_count(elm_count_cpo, std::ranges::subrange<It, S, K>&) -> constant<2>;


template<size_t I>
struct elm_t_cpo{};

// T can have any cv-ref qualifier
template<size_t I, class T>
using elm_t = decltype(dsk_elm_t(elm_t_cpo<I>(), std::declval<std::remove_cvref_t<T>&>()));


template<size_t I, class... T        > auto dsk_elm_t(elm_t_cpo<I>, tuple<T...>&      ) -> type_pack_elm<I, T...>;
template<size_t I, class T, size_t N > auto dsk_elm_t(elm_t_cpo<I>, T(&)[N]           ) -> T;
template<size_t I, class T           > auto dsk_elm_t(elm_t_cpo<I>, T&                ) -> std::tuple_element_t<I, T>; // won't match types derived from a std tuple like
template<size_t I, class... T        > auto dsk_elm_t(elm_t_cpo<I>, std::tuple<T...>& ) -> type_pack_elm<I, T...>;
template<size_t I, class T1, class T2> auto dsk_elm_t(elm_t_cpo<I>, std::pair<T1, T2>&) -> type_pack_elm<I, T1, T2>;
template<size_t I, class T, size_t N > auto dsk_elm_t(elm_t_cpo<I>, std::array<T, N>& ) -> T;
template<size_t I, class T           > auto dsk_elm_t(elm_t_cpo<I>, std::complex<T>&  ) -> T;
template<size_t I, class It, class S, std::ranges::subrange_kind K>
auto dsk_elm_t(elm_t_cpo<I>, std::ranges::subrange<It, S, K>&) -> type_pack_elm<I, It, S>;


template<class T>
concept _tuple_like_ = requires{ dsk_elm_count(elm_count_cpo(), std::declval<std::remove_cvref_t<T>&>()); /*get_elm<0>(t); tuple<> doesn't support this*/ };


// unified tuple or other type access:
// other type will be treated like a tuple of single element which is itself.

template<size_t I>
constexpr auto&& get_uelm(auto&& t) noexcept
{
    if constexpr(_tuple_like_<decltype(t)>)
    {
        return get_elm<I>(DSK_FORWARD(t));
    }
    else
    {
        static_assert(I == 0);
        return DSK_FORWARD(t);
    }
}

template<class T>
inline constexpr size_t uelm_count = DSK_CONDITIONAL_V_NO_CAPTURE(_tuple_like_<T>, elm_count<T>, 1);


template<size_t I, class T>
using uelm_t = DSK_CONDITIONAL_T(_tuple_like_<T>, DSK_PARAM(elm_t<I, T>), std::remove_cvref_t<T>);


// 'ts' can be _tuple_like_ or other type,
// if you want to force other type to be a reference in result tuple, wrap it in fwd_as_tuple().
constexpr auto tuple_cat(auto&&... ts)
{
    constexpr auto n = (... + uelm_count<decltype(ts)>);

    constexpr auto outterIndices = []()
    {
        ct_index_vector<n> r;
        size_t i = 0;
        (... , r.append_range(make_index_array<uelm_count<decltype(ts)>>(i++)));
        return r;
    }();

    constexpr auto innerIndices = []()
    {
        ct_index_vector<n> r;
        (... , r.append_range(make_index_array<uelm_count<decltype(ts)>>()));
        return r;
    }();

    static_assert(outterIndices.size() == innerIndices.size());

    auto tt = fwd_as_tuple(DSK_FORWARD(ts)...);

    return apply_ct_elms2<outterIndices, innerIndices>
    (
        [&]<auto... Outter, auto... Inner>(constant_seq<Outter...>, constant_seq<Inner...>)
        {
            return tuple<
                uelm_t<Inner, uelm_t<Outter, decltype(tt)>>...
            >(get_uelm<Inner>(get_uelm<Outter>(tt))...);
        }
    );
}

constexpr auto tuple_cat() noexcept
{
    return tuple();
}


template<auto Indices>
constexpr decltype(auto) apply_elms(auto&& t, auto&& f)
{
    return apply_ct_elms<Indices>([&]<auto... I>() -> decltype(auto)
    {
        return may_invoke_with_nts<I...>(f, get_uelm<I>(DSK_FORWARD(t))...);
    });
}

constexpr decltype(auto) apply_elms(auto&& t, auto&& f)
{
    return apply_elms
    <
        make_index_array<uelm_count<decltype(t)>>()
    >
    (DSK_FORWARD(t), DSK_FORWARD(f));
}


template<size_t Lower, size_t Upper>
constexpr decltype(auto) apply_elms(auto&& t, auto&& f)
{
    static_assert(Lower <= Upper);
    static_assert(Upper <= uelm_count<decltype(t)>);

    return apply_elms<make_index_array<Lower, Upper>()>(DSK_FORWARD(t), DSK_FORWARD(f));
}

template<size_t N>
constexpr decltype(auto) apply_first_n_elms(auto&& t, auto&& f)
{
    return apply_elms<0, N>(DSK_FORWARD(t), DSK_FORWARD(f));
}

template<size_t N>
constexpr decltype(auto) apply_last_n_elms(auto&& t, auto&& f)
{
    constexpr size_t nElms = uelm_count<decltype(t)>;

    static_assert(N <= nElms);

    return apply_elms<nElms - N, nElms>(DSK_FORWARD(t), DSK_FORWARD(f));
}


template<auto Indices>
constexpr decltype(auto) apply_elms_with_types(auto&& t, auto&& f)
{
    return apply_ct_elms<Indices>([&]<auto... I>() -> decltype(auto)
    {
        return f.template operator()<uelm_t<I, decltype(t)>...>(get_uelm<I>(DSK_FORWARD(t))...);
    });
}

constexpr decltype(auto) apply_elms_with_types(auto&& t, auto&& f)
{
    return apply_elms_with_types
    <
        make_index_array<uelm_count<decltype(t)>>()
    >
    (DSK_FORWARD(t), DSK_FORWARD(f));
}


// return a tuple of elems transformed by 'f'
template<auto Indices, template<class...> class R = tuple>
constexpr auto transform_elms(auto&& t, auto&& f)
{
    return apply_ct_elms<Indices>([&]<size_t... I>()
    {
        return R<decltype(may_invoke_with_nts<I>(f, get_uelm<I>(DSK_FORWARD(t))))...>(
                          may_invoke_with_nts<I>(f, get_uelm<I>(DSK_FORWARD(t)))...);
    });
}

template<template<class...> class R = tuple>
constexpr auto transform_elms(auto&& t, auto&& f)
{
    return transform_elms
    <
        make_index_array<uelm_count<decltype(t)>>(),
        R
    >
    (DSK_FORWARD(t), DSK_FORWARD(f));
}


template<auto Indices>
constexpr decltype(auto) foreach_elms(auto&& t, auto&& f)
{
    return foreach_ct_elms<Indices>([&]<auto I>() -> decltype(auto)
    {
        return may_invoke_with_nts<I>(f, get_uelm<I>(DSK_FORWARD(t)));
    });
}

constexpr decltype(auto) foreach_elms(auto&& t, auto&& f)
{
    return foreach_elms
    <
        make_index_array<uelm_count<decltype(t)>>()
    >
    (DSK_FORWARD(t), DSK_FORWARD(f));
}


template<auto Indices, class EmptyRet = void>
constexpr decltype(auto) foreach_elms_until(auto&& t, auto&& f, auto&& pred, auto&&... onNotFound)
{
    return foreach_ct_elms_until<Indices, EmptyRet>
    (
        [&]<auto I>() -> decltype(auto)
        {
            return may_invoke_with_nts<I>(f, get_uelm<I>(DSK_FORWARD(t)));
        },
        DSK_FORWARD(pred),
        DSK_FORWARD(onNotFound)...
    );
}

template<class EmptyRet = void>
constexpr decltype(auto) foreach_elms_until(auto&& t, auto&& f, auto&& pred, auto&&... onNotFound)
{
    return foreach_elms_until
    <
        make_index_array<uelm_count<decltype(t)>>(),
        EmptyRet
    >
    (DSK_FORWARD(t), DSK_FORWARD(f), DSK_FORWARD(pred), DSK_FORWARD(onNotFound)...);
}


constexpr auto make_tuple_from_tuple(auto&& t)
{
    return apply_elms(DSK_FORWARD(t), [](auto&&... elms)
    {
        return tuple(DSK_FORWARD(elms)...);
    });
}


template<auto Indices>
constexpr decltype(auto) visit_elm(size_t i, auto&& t, auto&& f)
{
    return visit_ct_elm<Indices>(i, [&]<auto I>() -> decltype(auto)
    {
        return may_invoke_with_nts<I>(f, get_uelm<I>(DSK_FORWARD(t)));
    });
}

constexpr decltype(auto) visit_elm(size_t i, auto&& t, auto&& f)
{
    return visit_elm
    <
        make_index_array<uelm_count<decltype(t)>>()
    >
    (i, DSK_FORWARD(t), DSK_FORWARD(f));
}



// Universal foreach.
constexpr decltype(auto) uforeach(auto&& u, auto&& f)
{
    if constexpr(_tuple_like_<decltype(u)>)
    {
        return foreach_elms(DSK_FORWARD(u), DSK_FORWARD(f));
    }
    else if constexpr(std::ranges::range<decltype(u)>)
    {
        for(auto&& e : DSK_FORWARD(u))
        {
            f(DSK_FORWARD(e));
        }
    }
    else
    {
        return f(DSK_FORWARD(u));
    }
}


constexpr void assert_equal_size(auto&& r0, auto&&... rs) noexcept
{
    if constexpr((_tuple_like_<decltype(r0)> && ... && _tuple_like_<decltype(rs)>))
    {
        static_assert((... && (elm_count<decltype(r0)> == elm_count<decltype(rs)>)));
    }
    else
    {
        DSK_ASSERT((... && (std::ranges::size(r0) == std::ranges::size(rs))));
    }
}

template<size_t N>
constexpr void assert_min_size(auto&& r) noexcept
{
    if constexpr(_tuple_like_<decltype(r)>)
    {
        static_assert(elm_count<decltype(r)> >= N);
    }
    else
    {
        DSK_ASSERT(std::ranges::size(r) >= N);
    }
}


enum elm_sel_cat
{
    by_ref  , // selected elms are stored as ref in returned tuple
    by_value, // selected elms are copied or moved into returned tuple
    by_copy , // selected elms are copied into returned tuple
    by_move , // selected elms are moved into returned tuple
    by_tuple, // selected elms are tuples and their (referred) elements are copied or moved into tuples of returned tuple
};

template<auto Indices, elm_sel_cat Cat = by_ref>
constexpr auto select_elms(auto&& elms)
{
    return apply_elms<Indices>(DSK_FORWARD(elms), [](auto&&... es)
    {
             if constexpr(Cat == by_ref  ) return fwd_as_tuple(DSK_FORWARD(es)...);
        else if constexpr(Cat == by_value) return        tuple(DSK_FORWARD(es)...);
        else if constexpr(Cat == by_copy ) return        tuple(es...);
        else if constexpr(Cat == by_move ) return        tuple(std::move(es)...);
        else if constexpr(Cat == by_tuple) return        tuple(make_tuple_from_tuple(DSK_FORWARD(es))...);
        else
            static_assert(false, "unsupported category");
    });
}

template<size_t Lower, size_t Upper, elm_sel_cat Cat = by_ref>
constexpr auto select_elms(auto&& elms)
{
    static_assert(Lower <= Upper);
    static_assert(Upper <= uelm_count<decltype(elms)>);

    return select_elms
    <
        make_index_array<Lower, Upper>(),
        Cat
    >
    (DSK_FORWARD(elms));
}

template<elm_sel_cat Cat>
constexpr auto select_all_elms(auto&& elms)
{
    return select_elms<0, uelm_count<decltype(elms)>, Cat>(DSK_FORWARD(elms));
}

template<size_t N, elm_sel_cat Cat = by_ref>
constexpr auto select_first_n_elms(auto&& elms)
{
    return select_elms<0, N, Cat>(DSK_FORWARD(elms));
}

template<size_t N, elm_sel_cat Cat = by_ref>
constexpr auto select_last_n_elms(auto&& elms)
{
    constexpr size_t nElms = uelm_count<decltype(elms)>;

    static_assert(N <= nElms);

    return select_elms<nElms - N, nElms, Cat>(DSK_FORWARD(elms));
}


template<size_t N, elm_sel_cat Cat = by_ref>
constexpr auto remove_first_n_elms(auto&& elms)
{
    return select_elms<N, uelm_count<decltype(elms)>, Cat>(DSK_FORWARD(elms));
}

template<size_t N, elm_sel_cat Cat = by_ref>
constexpr auto remove_last_n_elms(auto&& elms)
{
    constexpr size_t nElms = uelm_count<decltype(elms)>;

    static_assert(N <= nElms);

    return select_elms<0, nElms - N>, Cat>(DSK_FORWARD(elms));
}


template<auto IdxParts, elm_sel_cat Cat = by_ref>
constexpr auto group_elms(auto&& elms)
{
    return apply_ct_elms<IdxParts>([&]<auto... Indices>()
    {
        return tuple(select_elms<Indices, Cat>(DSK_FORWARD(elms))...);
    });
}


constexpr auto to_fwded_tuple(auto&& elms) noexcept
{
    return select_all_elms<by_ref>(DSK_FORWARD(elms));
}


} // namespace dsk


namespace std{

// structured binding support

template<class... T>
struct tuple_size<dsk::tuple<T...>>
    : std::integral_constant<size_t, sizeof...(T)>
{};

template<size_t I, class... T>
struct tuple_element<I, dsk::tuple<T...>>
    : std::type_identity<dsk::type_pack_elm<I, T...>>
{};

} // namespace std
