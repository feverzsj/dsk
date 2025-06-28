#pragma once

#include <dsk/util/tuple.hpp>
#include <dsk/util/ct_vector.hpp>
#include <dsk/util/type_name.hpp>
#include <dsk/util/type_pack_elm.hpp>
#include <ranges>
#include <utility>
#include <algorithm>


namespace dsk{


template<class... T>
struct type_list_t
{
    template<size_t I>
    using type = type_pack_elm<I, T...>;

    template<template<class...> class L, class... U>
    constexpr auto operator+(L<U...>) const noexcept
    {
        return type_list_t<T..., U...>();
    }

    template<bool Cond, class... U>
    using add_if_t = std::conditional_t<Cond, type_list_t<T..., U...>, type_list_t>;

    template<template<class...> class L>
    using to_t = L<T...>;

    static constexpr size_t count = sizeof...(T);
    static constexpr size_t size  = count;
};

template<class... T>
inline constexpr type_list_t<T...> type_list;


template<template<class...> class L, class... T> void _type_list_impl(L<T...>&);
template<class T> concept _type_list_ = requires(std::remove_cvref_t<T>& t){ dsk::_type_list_impl(t); };


template<template<class...> class L, class... T>
constexpr L<> empty_type_list_from(L<T...>) noexcept
{
    return {};
}

template<template<class...> class L, class... T>
constexpr size_t type_list_size(L<T...>) noexcept
{
    return sizeof...(T);
}

template<template<class...> class L, class... T>
constexpr size_t type_list_size(std::type_identity<L<T...>>) noexcept
{
    return sizeof...(T);
}

template<class L>
constexpr size_t type_list_size() noexcept
{
    return type_list_size(type_c<std::remove_cvref_t<L>>);
}


template<template<class...> class L, class... T>
constexpr auto to_type_list(L<T...>) noexcept
{
    return type_list<T...>;
}

template<template<class...> class L, class... T>
constexpr auto to_type_list(std::type_identity<L<T...>>) noexcept
{
    return type_list<T...>;
}

template<class L>
constexpr auto to_type_list() noexcept
{
    return to_type_list(type_c<std::remove_cvref_t<L>>);
}


template<template<class...> class L, class... T>
constexpr auto to_no_cvref_type_list(L<T...>) noexcept
{
    return type_list<std::remove_cvref_t<T>...>;
}

template<template<class...> class L, class... T>
constexpr auto to_no_cvref_type_list(std::type_identity<L<T...>>) noexcept
{
    return type_list<std::remove_cvref_t<T>...>;
}

template<class L>
constexpr auto to_no_cvref_type_list() noexcept
{
    return to_no_cvref_type_list(type_c<std::remove_cvref_t<L>>);
}


template<template<class...> class L, class... T>
constexpr auto unwrap_no_cvref_type_list(L<T...>) noexcept
{
    return type_list<unwrap_no_cvref_t<T>...>;
}

template<template<class...> class L, class... T>
constexpr auto unwrap_no_cvref_type_list(std::type_identity<L<T...>>) noexcept
{
    return type_list<unwrap_no_cvref_t<T>...>;
}

template<class L>
constexpr auto unwrap_no_cvref_type_list() noexcept
{
    return unwrap_no_cvref_type_list(type_c<std::remove_cvref_t<L>>);
}


template<_tuple_like_ T>
constexpr auto elms_to_type_list() noexcept
{
    return apply_ct_elms<make_index_array<elm_count<T>>()>([&]<auto... I>()
    {
        return type_list<elm_t<I, T>...>;
    });
}

template<class U>
constexpr auto uelms_to_type_list() noexcept
{
    return apply_ct_elms<make_index_array<uelm_count<U>>()>([&]<auto... I>()
    {
        return type_list<uelm_t<I, U>...>;
    });
}


template<class From, template<class...> class To>
using cvt_type_list_t = typename decltype(to_type_list<From>())::template to_t<To>;


// NOTE: using on type_identity<L> will always be treat as type list with single element,
//       instead, use function form directly foo(type_c<L>).
#define DSK_TYPE_LIST_SIZE(e) type_list_size<decltype(e)>()
#define DSK_TYPE_LIST(e) to_type_list<decltype(e)>()
#define DSK_NO_CVREF_TYPE_LIST(e) to_no_cvref_type_list<decltype(e)>()
#define DSK_UNWRAP_NO_CVREF_TYPE_LIST(e) unwrap_no_cvref_type_list<decltype(e)>()


template<auto Indices, template<class...> class L, class... T>
constexpr auto rearrange_type_list(L<T...>)
{
    return apply_ct_elms<Indices>([]<auto... I>{
        return L<type_pack_elm<I, T...>...>();
    });
}

template<auto Indices, class L>
constexpr auto rearrange_type_list(std::type_identity<L>)
{
    return rearrange_type_list<Indices>(to_type_list<L>());
}

template<auto Indices, class L>
constexpr auto rearrange_type_list()
{
    return rearrange_type_list<Indices>(to_type_list<L>());
}


template<class L>
constexpr auto select_even_types()
{
    return rearrange_type_list<make_even_index_array<type_list_size<L>()>(), L>();
}

template<class L>
constexpr auto select_even_types(std::type_identity<L>)
{
    return select_even_types<L>();
}

template<class L>
constexpr auto select_even_types(L)
{
    return select_even_types<L>();
}


template<class L>
constexpr auto select_odd_types()
{
    return rearrange_type_list<make_odd_index_array<type_list_size<L>()>(), L>();
}

template<class L>
constexpr auto select_odd_types(std::type_identity<L>)
{
    return select_odd_types<L>();
}

template<class L>
constexpr auto select_odd_types(L)
{
    return select_odd_types<L>();
}



inline constexpr auto by_type_name = []<class T>(){ return type_name<T>; };


template<auto I, class L>
constexpr auto invoke_ct_op(L, auto op)
{
    using T = DSK_CONDITIONAL_T(type_list_size<L>(), DSK_PARAM(type_list_elm<I, L>), int);

         if constexpr(requires{ op.template operator()<T      >(); }) return op.template operator()<T      >();
    else if constexpr(requires{ op.template operator()<   I   >(); }) return op.template operator()<   I   >();
    else if constexpr(requires{ op.template operator()<T, I   >(); }) return op.template operator()<T, I   >();
    else if constexpr(requires{ op.template operator()<   I, L>(); }) return op.template operator()<   I, L>();
    else                                                              return op.template operator()<T, I, L>();
}

template<auto Indices>
constexpr auto indexed_ct_op(auto l, auto op)
{
    constexpr size_t nIndices = std::size(Indices);
    constexpr size_t nTypes = type_list_size(l);

    if constexpr(! nIndices)
    {
        return [](auto){ static_assert(false, "should never be invoked"); };
    }
    else
    {
        static_assert(nTypes);

        constexpr auto evalIndices = DSK_CONDITIONAL_V
        (
            nIndices <= nTypes, Indices,
                                make_index_array<nTypes>()
        );

        std::array
        <
            decltype(invoke_ct_op<Indices[0]>(l, op)),
            nTypes
        >
        projs{};

        foreach_ct_elms<evalIndices>([&]<auto I>
        {
            projs[I] = invoke_ct_op<I>(l, op);
        });

        return [projs](auto i){ return projs[i]; };
    }
}

constexpr auto indexed_ct_op(auto l, auto op)
{
    return indexed_ct_op<make_index_array<type_list_size(l)>()>(l, op);
}


template<class Op, class... Arg>
struct ct_bound_op
{
    Op op;
    tuple<Arg...> args;

    // direct op
    constexpr auto operator()(auto l) const
    {
        return apply_elms(args, [&](auto&... arg){ return op(l, arg...); });
    }

    // chainable op
    template<auto Prev>
    constexpr auto operator()(auto l) const
    {
        return apply_elms(args, [&](auto&... arg){ return op.template invoke<Prev>(l, arg...); });
    }
};

template<class Derived>
struct ct_chainable_op_base
{
    constexpr Derived const& derived() const noexcept { return static_cast<Derived const&>(*this); }

    // direct op
    template<template<class...> class L, class... T>
    constexpr auto operator()(L<T...> l, auto... arg) const
    {
        return Derived::template invoke<make_index_array<type_list_size(l)>()>(l, arg...);
    }

    // bound op, the arg should match op.invoke()'s other paramteres
    constexpr auto operator()(auto... arg) const
    {
        return ct_bound_op<Derived, decltype(arg)...>
        {
            static_cast<Derived const&>(*this), tuple{arg...}
        };
    }
};


inline constexpr struct ct_all_of_t : ct_chainable_op_base<ct_all_of_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
            return true;
        else
            return std::ranges::all_of(Indices, indexed_ct_op<Indices>(l, pred));
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_all_of;

inline constexpr struct ct_any_of_t : ct_chainable_op_base<ct_any_of_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
            return false;
        else
            return std::ranges::any_of(Indices, indexed_ct_op<Indices>(l, pred));
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_any_of;

inline constexpr struct ct_none_of_t : ct_chainable_op_base<ct_none_of_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
            return true;
        else
            return std::ranges::none_of(Indices, indexed_ct_op<Indices>(l, pred));
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_none_of;

inline constexpr auto ct_contains = ct_any_of;

inline constexpr struct ct_count_t : ct_chainable_op_base<ct_count_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
            return std::ranges::range_difference_t<decltype(Indices)>(0);
        else
            return std::ranges::count_if(Indices, indexed_ct_op<Indices>(l, pred));
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_count;

inline constexpr struct ct_find_first_t : ct_chainable_op_base<ct_find_first_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 1>;

            auto it = std::ranges::find_if(Indices, indexed_ct_op<Indices>(l, pred));

            return (it != std::ranges::end(Indices))? seq_t{*it} : seq_t();
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_find_first;

inline constexpr struct ct_find_first_not_t : ct_chainable_op_base<ct_find_first_not_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 1>;

            auto it = std::ranges::find_if_not(Indices, indexed_ct_op<Indices>(l, pred));

            return (it != std::ranges::end(Indices))? seq_t{*it} : seq_t();
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_find_first_not;



#ifdef __cpp_lib_ranges_find_last
#define _find_last_if std::ranges::find_last_if
#define _find_last_if_not std::ranges::find_last_if_not
#else
auto _find_last_if(auto&& r, auto&& pred)
{
    return std::ranges::find_if(reverse_range(DSK_FORWARD(r)), DSK_FORWARD(pred));
}
auto _find_last_if_not(auto&& r, auto&& pred)
{
    return std::ranges::find_if_not(reverse_range(DSK_FORWARD(r)), DSK_FORWARD(pred));
}
#endif


inline constexpr struct ct_find_last_t : ct_chainable_op_base<ct_find_last_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 1>;

            auto [it, end] = _find_last_if(Indices, indexed_ct_op<Indices>(l, pred));

            return (it != end) ? seq_t{*it} : seq_t();
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_find_last;

inline constexpr struct ct_find_last_not_t : ct_chainable_op_base<ct_find_last_not_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 1>;

            auto [it, end] = _find_last_if_not(Indices, indexed_ct_op<Indices>(l, pred));

            return (it != end) ? seq_t{*it} : seq_t();
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_find_last_not;


inline constexpr struct ct_find_end_t : ct_chainable_op_base<ct_find_end_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj1, auto proj2)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                std::ranges::find_end(Indices, make_index_array<type_list_size(r)>(),
                                      std::ranges::equal_to(),
                                      indexed_ct_op<Indices>(l, proj1), indexed_ct_op(r, proj2))
            );
        }
    }
} ct_find_end;


inline constexpr struct ct_find_first_of_t : ct_chainable_op_base<ct_find_first_of_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj1, auto proj2)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 1>;

            auto it = std::ranges::find_first_of(Indices, make_index_array<type_list_size(r)>(),
                                                 std::ranges::equal_to(),
                                                 indexed_ct_op<Indices>(l, proj1), indexed_ct_op(r, proj2));

            return (it != std::ranges::end(Indices)) ? seq_t{*it} : seq_t();
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj)
    {
        return invoke<Indices>(l, r, proj, proj);
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r)
    {
        return invoke<Indices>(l, r, by_type_name);
    }
} ct_find_first_of;


inline constexpr struct ct_adjacent_find_t : ct_chainable_op_base<ct_adjacent_find_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            using seq_t = ct_vector<std::ranges::range_value_t<decltype(Indices)>, 2>;

            auto it = std::ranges::adjacent_find(Indices, std::ranges::equal_to(), indexed_ct_op<Indices>(l, proj));
        
            return (it != std::ranges::end(Indices)) ? seq_t{*it, *std::ranges::next(it)} : seq_t();
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l)
    {
        return invoke<Indices>(l, by_type_name);
    }
} ct_adjacent_find;


inline constexpr struct ct_search_t : ct_chainable_op_base<ct_search_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj1, auto proj2)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                std::ranges::search(Indices, make_index_array<type_list_size(r)>(),
                                    std::ranges::equal_to(),
                                    indexed_ct_op<Indices>(l, proj1), indexed_ct_op(r, proj2))
            );
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj)
    {
        return invoke<Indices>(l, r, proj, proj);
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r)
    {
        return invoke<Indices>(l, r, by_type_name);
    }
} ct_search;

inline constexpr struct ct_search_n_t : ct_chainable_op_base<ct_search_n_t>
{
    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, auto n, U<V>)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                std::ranges::search_n(Indices, n, type_name<V>,
                                      std::ranges::equal_to(),
                                      indexed_ct_op<Indices>(l, by_type_name))
            );
        }
    }
} ct_search_n;


inline constexpr struct ct_begin_with_t : ct_chainable_op_base<ct_begin_with_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto... preds)
    {
        if constexpr(std::size(Indices) >= sizeof...(preds))
        {
            size_t i = 0;
            return (... && indexed_ct_op<Indices>(l, preds)(Indices[i++]));
        }
        else
        {
            return false;
        }
    }
} ct_begin_with;


#ifdef __cpp_lib_ranges_starts_ends_with

inline constexpr struct ct_starts_with_t : ct_chainable_op_base<ct_starts_with_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj1, auto proj2)
    {
        if constexpr(std::empty(Indices))
        {
            return false;
        }
        else
        {
            return std::ranges::starts_with(Indices, make_index_array<type_list_size(r)>(),
                                            std::ranges::equal_to(),
                                            indexed_ct_op<Indices>(l, proj1), indexed_ct_op(r, proj2));
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj)
    {
        return invoke<Indices>(l, r, proj, proj);
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r)
    {
        return invoke<Indices>(l, r, by_type_name);
    }
} ct_starts_with;

inline constexpr struct ct_ends_with_t : ct_chainable_op_base<ct_ends_with_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj1, auto proj2)
    {
        if constexpr(std::empty(Indices))
        {
            return false;
        }
        else
        {
            return std::ranges::ends_with(Indices, make_index_array<type_list_size(r)>(),
                                          std::ranges::equal_to(),
                                          indexed_ct_op<Indices>(l, proj1), indexed_ct_op(r, proj2));
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r, auto proj)
    {
        return invoke<Indices>(l, r, proj, proj);
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto r)
    {
        return invoke<Indices>(l, r, by_type_name);
    }
} ct_ends_with;

#endif // __cpp_lib_ranges_starts_ends_with


inline constexpr struct ct_keep_t : ct_chainable_op_base<ct_keep_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            > indices;

            std::ranges::copy_if(Indices, std::back_inserter(indices), indexed_ct_op<Indices>(l, pred));

            return indices;
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_keep;

inline constexpr struct ct_remove_t : ct_chainable_op_base<ct_remove_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            auto indices = ct_vector(Indices);

            auto [ret, last] = std::ranges::remove_if(indices, indexed_ct_op<Indices>(l, pred));

            indices.erase(ret, last);

            return indices;
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_remove;

inline constexpr struct ct_reverse_t : ct_chainable_op_base<ct_reverse_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto)
    {
        if constexpr(std::size(Indices) < 2)
        {
            return Indices;
        }
        else
        {
            auto indices = Indices;

            std::ranges::reverse(indices);

            return indices;
        }
    }
} ct_reverse;

inline constexpr struct ct_rotate_t : ct_chainable_op_base<ct_rotate_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto /*l*/, size_t middle)
    {
        if constexpr(std::size(Indices) < 2)
        {
            return Indices;
        }
        else
        {
            auto indices = Indices;

            std::ranges::rotate(indices, std::begin(indices) + middle);

            return indices;
        }
    }
} ct_rotate;


template<class Pred>
struct ct_sort_t : ct_chainable_op_base<ct_sort_t<Pred>>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj)
    {
        if constexpr(std::size(Indices) < 2)
        {
            return Indices;
        }
        else
        {
            auto indices = Indices;

            std::ranges::sort(indices, Pred(), indexed_ct_op<Indices>(l, proj));

            return indices;
        }
    }
};

inline constexpr ct_sort_t<std::ranges::less   > ct_sort;
inline constexpr ct_sort_t<std::ranges::greater> ct_sort_reverse;


// removes duplicate elements
inline constexpr struct ct_unique_t : ct_chainable_op_base<ct_unique_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj)
    {
        if constexpr(std::size(Indices) < 2)
        {
            return Indices;
        }
        else
        {
            auto uniuqeIndices = ct_vector(Indices);
            uniuqeIndices.clear();

            auto indexedProj = indexed_ct_op<Indices>(l, proj);

            for(auto I : Indices)
            {
                if(std::ranges::find(uniuqeIndices, indexedProj(I), std::ranges::equal_to(), indexedProj)
                   == std::ranges::end(Indices))
                {
                    uniuqeIndices.push_back(I);
                }
            }

            return uniuqeIndices;
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l)
    {
        return invoke<Indices>(l, by_type_name);
    }
} ct_unique;


inline constexpr struct ct_is_unique_t : ct_chainable_op_base<ct_is_unique_t>
{
    template<auto Indices>
    static constexpr bool invoke(auto l, auto proj)
    {
        if constexpr(std::size(Indices) > 1)
        {
            auto indexedProj = indexed_ct_op<Indices>(l, proj);

            for(auto prev = std::ranges::begin(Indices), it = prev + 1, end = std::ranges::end(Indices);
                it != end; prev = it, ++it)
            {
                if(std::ranges::find(it, end, indexedProj(*prev), indexedProj) != end)
                {
                    return false;
                }
            }
        }

        return true;
    }

    template<auto Indices>
    static constexpr auto invoke(auto l)
    {
        return invoke<Indices>(l, by_type_name);
    }
} ct_is_unique;


// partition list into sizeof...(preds) + 1 groups;
// NOTE: ct_partition is stable, as it uses std::ranges::partition_copy.
inline constexpr struct ct_partition_t : ct_chainable_op_base<ct_partition_t>
{
    template<auto Indices, class Seq, size_t I>
    static constexpr void do_apply(auto& grps, auto l, auto pred, auto... preds)
    {
        if constexpr(std::size(Indices))
        {
            constexpr auto seqs = [&]()
            {
                std::array<Seq, 2> seqs;

                std::ranges::partition_copy(Indices,
                    std::back_inserter(seqs[0]),
                    std::back_inserter(seqs[1]),
                    indexed_ct_op<Indices>(l, pred));

                return seqs;
            }();
            
            grps[I] = seqs[0];

            if constexpr(sizeof...(preds))
            {
                do_apply<seqs[1], Seq, I + 1>(grps, l, preds...);
            }
            else
            {
                static_assert(I + 2 == elm_count<decltype(grps)>);
                grps[I + 1] = seqs[1];
            }
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto... preds)
    {
        if constexpr(sizeof...(preds))
        {
            using seq_t = ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)>;

            std::array<seq_t, sizeof...(preds) + 1> grps;

            do_apply<Indices, seq_t, 0>(grps, l, preds...);

            return grps;
        }
        else
        {
            return std::array{Indices};
        }
    }
} ct_partition;


inline constexpr struct ct_split_at_first_n_t : ct_chainable_op_base<ct_split_at_first_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto /*l*/, size_t n)
    {
        constexpr size_t nIdx = std::size(Indices);

        assert(n <= nIdx);

        using seq_t = ct_vector<
            std::ranges::range_value_t<decltype(Indices)>,
            nIdx>;

        auto beg = std::begin(Indices);

        return std::array<seq_t, 2>{
            seq_t(beg    , beg + n),
            seq_t(beg + n, std::end(Indices))
        };
    }
} ct_split_at_first_n;


inline constexpr struct ct_split_at_last_n_t : ct_chainable_op_base<ct_split_at_last_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, size_t n)
    {
        constexpr size_t nIdx = std::size(Indices);

        assert(n <= nIdx);

        return ct_split_at_first_n_t::invoke<Indices>(l, nIdx - n);
    }
} ct_split_at_last_n;


inline constexpr struct ct_split_at_first_t : ct_chainable_op_base<ct_split_at_first_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return std::array{Indices, Indices};
        }
        else
        {
            using seq_t = ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)>;

            auto it = std::ranges::find_if(Indices, indexed_ct_op<Indices>(l, pred));

            return std::array<seq_t, 2>{
                seq_t(std::begin(Indices), it),
                seq_t(it, std::end(Indices))
            };
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_split_at_first;


inline constexpr struct ct_until_first_t : ct_chainable_op_base<ct_until_first_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            auto it = std::ranges::find_if(Indices, indexed_ct_op<Indices>(l, pred));

            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                std::ranges::begin(Indices), it
            );
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_until_first;


inline constexpr struct ct_until_first_not_t : ct_chainable_op_base<ct_until_first_not_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            auto it = std::ranges::find_if_not(Indices, indexed_ct_op<Indices>(l, pred));

            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                std::ranges::begin(Indices), it
            );
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_until_first_not;


inline constexpr struct ct_start_at_first_t : ct_chainable_op_base<ct_start_at_first_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred)
    {
        if constexpr(std::empty(Indices))
        {
            return Indices;
        }
        else
        {
            auto it = std::ranges::find_if(Indices, indexed_ct_op<Indices>(l, pred));

            return ct_vector<
                std::ranges::range_value_t<decltype(Indices)>,
                std::size(Indices)
            >(
                it, std::end(Indices)
            );
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)));
    }
} ct_start_at_first;


inline constexpr struct keep_delimiter_t{} keep_delimiter;
inline constexpr struct keep_empty_seg_t{} keep_empty_seg;

inline constexpr struct ct_split_t : ct_chainable_op_base<ct_split_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto pred, auto... opts)
    {
        if constexpr(std::empty(Indices))
        {
            return empty_range<decltype(Indices)>;
        }
        else
        {
            constexpr auto optList = type_list<decltype(opts)...>;
            constexpr bool keepDelimiter = ct_any_of(optList, DSK_TYPE_EXP((std::is_same_v<T, keep_delimiter_t>)));
            constexpr bool keepEmptySeg  = ct_any_of(optList, DSK_TYPE_EXP((std::is_same_v<T, keep_empty_seg_t>)));

            ct_vector<
                ct_vector<
                    std::ranges::range_value_t<decltype(Indices)>,
                    std::size(Indices)
                >,
                std::size(Indices)
            > r;

            auto f = indexed_ct_op<Indices>(l, pred);

            auto beg = std::begin(Indices);
            auto end = std::end(Indices);

            for(; beg != end;)
            {
                auto it = std::ranges::find_if(beg, end, f);

                if(it == end)
                    break;
            
                if constexpr(keepEmptySeg)
                {
                    r.emplace_back().append(beg, it);
                }
                else
                {
                    if(beg != it) // won't add empty segment
                        r.emplace_back().append(beg, it);
                }

                if constexpr(keepDelimiter)
                    r.emplace_back().emplace_back(*it);

                beg = ++it;
            }

            if(beg != end)
                r.emplace_back().append(beg, end);

            return r;
        }
    }

    template<auto Indices, template<class> class U, class V>
    static constexpr auto invoke(auto l, U<V>, auto... opts)
    {
        return invoke<Indices>(l, DSK_TYPE_EXP((std::is_same_v<T, V>)), opts...);
    }
} ct_split;


inline constexpr struct ct_select_t : ct_chainable_op_base<ct_select_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto /*l*/, size_t lower, size_t upper)
    {
        constexpr size_t nIdx = std::size(Indices);

        assert(lower <= upper);
        assert(upper <= nIdx);

        ct_vector<
            std::ranges::range_value_t<decltype(Indices)>,
            nIdx
        > r;

        for(auto i = lower; i < upper; ++i)
        {
            r.push_back(Indices[i]);
        }

        return r;
    }
} ct_select;


inline constexpr struct ct_select_first_n_t : ct_chainable_op_base<ct_select_first_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, size_t n)
    {
        return ct_select_t::invoke<Indices>(l, 0, n);
    }
} ct_select_first_n;


inline constexpr struct ct_select_last_n_t : ct_chainable_op_base<ct_select_last_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, size_t n)
    {
        constexpr size_t nIdx = std::size(Indices);

        assert(n <= nIdx);

        return ct_select_t::invoke<Indices>(l, nIdx - n, nIdx);
    }
} ct_select_last_n;


inline constexpr struct ct_remove_first_n_t : ct_chainable_op_base<ct_remove_first_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, size_t n)
    {
        return ct_select_t::invoke<Indices>(l, n, std::size(Indices));
    }
} ct_remove_first_n;


inline constexpr struct ct_remove_last_n_t : ct_chainable_op_base<ct_remove_last_n_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, size_t n)
    {
        constexpr size_t nIdx = std::size(Indices);

        assert(n <= nIdx);

        return ct_select_t::invoke<Indices>(l, 0, nIdx - n);
    }
} ct_remove_last_n;


inline constexpr struct ct_accumulate_t : ct_chainable_op_base<ct_accumulate_t>
{
    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj, auto init, auto binaryOp)
    {
        if constexpr(std::empty(Indices))
        {
            return init;
        }
        else
        {
            auto indexedProj = indexed_ct_op<Indices>(l, proj);

            for(auto I : Indices)
            {
                init = binaryOp(init, indexedProj(I)); ;
            }

            return init;
        }
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj, auto init)
    {
        return invoke<Indices>(l, proj, init, std::plus<void>());
    }

    template<auto Indices>
    static constexpr auto invoke(auto l, auto proj)
    {
        return invoke<Indices>(l, proj, static_cast<size_t>(0));
    }
} ct_accumulate;


// invoke chainable ops to l, returns what last op would return.
// a chainable op is something like:
//
//     []<auto PrevOpReturn>(auto l)
//     {
//         ....
//         return passToNextOp;
//     }
//
// usage:
//     ct_chained_invoke(type_list<int, float, int, double, char, char>,
//                       ct_sort(by_type_name),
//                       ct_unique(by_type_name),
//                       ct_count(type_c<int>));
//
template<template<class...> class L, class... T>
constexpr auto ct_chained_invoke(L<T...> l, auto... f)
{
    return _do_ct_chained_invoke<make_index_array<sizeof...(T)>()>(l, f...);
}

template<auto PrevRet>
constexpr auto _do_ct_chained_invoke(auto l, auto f0)
{
    return f0.template operator()<PrevRet>(l);
}

template<auto PrevRet>
constexpr auto _do_ct_chained_invoke(auto l, auto f0, auto... f)
{
    return _do_ct_chained_invoke<f0.template operator()<PrevRet>(l)>(l, f...);
}


template<elm_sel_cat Cat = by_ref>
constexpr auto keep_elms(auto&& elms, auto pred) noexcept
{
    return select_elms<
        ct_keep(DSK_TYPE_LIST(elms), pred),
        Cat
    >(DSK_FORWARD(elms));
}


template<elm_sel_cat Cat = by_ref>
constexpr auto split_elms_at_first(auto&& elms, auto pred) noexcept
{
    return group_elms<
        ct_split_at_first(DSK_TYPE_LIST(elms), pred),
        Cat
    >(DSK_FORWARD(elms));
}

template<elm_sel_cat Cat = by_ref>
constexpr auto select_elms_until_first(auto&& elms, auto pred) noexcept
{
    return select_elms<
        ct_until_first(DSK_TYPE_LIST(elms), pred),
        Cat
    >(DSK_FORWARD(elms));
}

template<elm_sel_cat Cat = by_ref>
constexpr auto select_elms_start_at_first(auto&& elms, auto pred) noexcept
{
    return select_elms<
        ct_start_at_first(DSK_TYPE_LIST(elms), pred),
        Cat
    >(DSK_FORWARD(elms));
}

template<elm_sel_cat Cat = by_ref>
constexpr auto partition_elms(auto&& elms, auto... preds) noexcept
{
    return group_elms<
        ct_partition(DSK_TYPE_LIST(elms), preds...),
        Cat
    >(DSK_FORWARD(elms));
}

template<elm_sel_cat Cat = by_ref>
constexpr auto partition_args(auto pred, auto&&... args) noexcept
{
    return partition_elms<Cat>(fwd_as_tuple(DSK_FORWARD(args)...), pred);
}


/// type manipulate

template<auto Indices, template<class...> class L, class... T>
constexpr auto ct_apply(L<T...>, auto f)
{
    return apply_ct_elms<Indices>([&]<auto... I>()
    {
        return f.template operator()<type_pack_elm<I, T...>...>();
    });
}

template<auto Indices, class L>
constexpr auto ct_apply(auto f)
{
    return ct_apply(to_type_list<L>(), f);
}

template<auto Indices, class L>
constexpr auto ct_apply(std::type_identity<L>, auto f)
{
    return ct_apply<Indices, L>(f);
}


constexpr auto ct_apply(auto l, auto f)
{
    return ct_apply<make_index_array<type_list_size(l)>()>(l, f);
}

template<class L>
constexpr auto ct_apply(auto f)
{
    return ct_apply(type_c<L>, f);
}


// usage:
//   ct_transform(L<T...>(), []<class T>() { return type_c<T const>; }); // returns L<T const>();
//   ct_transform(type_c<L<T...>>, []<class T>() { return type_c<T const>; }); // returns std::type_identity<L<T const...>>
//   ct_transform<L<T...>>([]<class T>() { return type_c<T const>; }); // returns std::type_identity<L<T const...>>

template<auto Indices, template<class...> class L, class... T>
constexpr auto ct_transform(L<T...> l, auto f)
{
    return apply_ct_elms<Indices>([]<auto... I>()
    {
        return L<typename decltype(invoke_ct_op<I>(l, f))::type ...>();
    });
}

template<auto Indices, template<class...> class L, class... T>
constexpr auto ct_transform(std::type_identity<L<T...>>, auto f)
{
    return type_c<
        typename decltype(
            ct_transform<Indices>(type_list<T...>, f)
        )::template to_t<L>>;
}

template<auto Indices, class L>
constexpr auto ct_transform(auto f)
{
    return ct_transform<Indices>(type_c<L>, f);
}


constexpr auto ct_transform(auto l, auto f)
{
    return ct_transform<make_index_array<type_list_size(l)>()>(l, f);
}

template<class L>
constexpr auto ct_transform(auto f)
{
    return ct_transform(type_c<L>, f);
}



// returns concatenated type_list_t of invoke_ct_op<I>(l, f)
// f should return type_list_t
template<auto Indices, template<class...> class L, class... T>
constexpr auto ct_flatten(L<T...> l, auto f)
{
    return apply_ct_elms<Indices>([&]<auto... I>()
    {
        return (type_list<> + ... + invoke_ct_op<I>(l, f));
    });
}

template<auto Indices, class L>
constexpr auto ct_flatten(auto f)
{
    return ct_flatten(to_type_list<L>(), f);
}

template<auto Indices, class L>
constexpr auto ct_flatten(std::type_identity<L>, auto f)
{
    return ct_flatten<Indices, L>(f);
}


constexpr auto ct_flatten(auto l, auto f)
{
    return ct_flatten<make_index_array<type_list_size(l)>()>(l, f);
}

template<class L>
constexpr auto ct_flatten(auto f)
{
    return ct_flatten(type_c<L>, f);
}


} // namespace dsk
