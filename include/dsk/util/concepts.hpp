#pragma once

#include <dsk/config.hpp>
#include <dsk/util/type_traits.hpp>
#include <climits>
#include <tuple>
#include <ranges>
#include <memory>
#include <optional>
#include <concepts>
#include <functional>


namespace dsk{


// In Most Cases:
//   for type traits, apply std::remove_cv_t on the tested type;
//   for concepts, apply std::remove_cvref_t on the tested type;


template<class T>
class indirect_ref
{
    T** _r = nullptr;

public:
    using type = T;

    constexpr explicit indirect_ref(T*& p) : _r(&p) {}
    constexpr T& get() const noexcept { DSK_ASSERT(_r && *_r); return **_r; }
    constexpr operator T&() const noexcept { return get(); }
};


struct is_ref_wrap_cpo{};
template<class T> T* dsk_is_ref_wrap(is_ref_wrap_cpo, std::reference_wrapper<T>&);
template<class T> T* dsk_is_ref_wrap(is_ref_wrap_cpo,           indirect_ref<T>&);

// for lval-ref wrapper only
template<class W         > concept _ref_wrap_    = requires(std::remove_cvref_t<W>& w){  dsk_is_ref_wrap(is_ref_wrap_cpo(), w); };
template<class W, class T> concept _ref_wrap_of_ = requires(std::remove_cvref_t<W>& w){ {dsk_is_ref_wrap(is_ref_wrap_cpo(), w)} -> std::convertible_to<T*>; };

template<class T> using unwrap_ref_t = typename std::conditional_t<_ref_wrap_<T>, std::remove_cvref<T>, std::type_identity<T>>::type;
template<class T> using unwrap_no_cvref_t = std::remove_cvref_t<unwrap_ref_t<T>>;

constexpr auto&& unwrap_ref(auto&& t) noexcept
{
    if constexpr(_ref_wrap_<decltype(t)>) return t.get();
    else                                  return DSK_FORWARD(t);
}

constexpr auto&& unwrap_ref_or_move(auto& t) noexcept
{
    if constexpr(_ref_wrap_<decltype(t)>) return t.get();
    else                                  return mut_move(t);
}


constexpr decltype(auto) wrap_lref_fwd_other(auto&& t) noexcept
{
    if constexpr(std::is_lvalue_reference_v<decltype(t)>) return std::ref(t);
    else                                                  return DSK_FORWARD(t);
}

template<bool Cond>
constexpr decltype(auto) wrap_lref_if(auto&& t) noexcept
{
    if constexpr(Cond) return std::ref(DSK_FORWARD(t));
    else               return DSK_FORWARD(t);
}



template<class From, class To, class... Tos>
concept _convertible_to_one_of_ = (std::convertible_to<From, To> || ... || std::convertible_to<From, Tos>);

// xxx_ref_wrap<A> can only convert to A.
// If A can convert to B, xxx_ref_wrap<A> can not convert to B;
// _unwrap_convertible_to_<xxx_ref_wrap<A>, B> == true;
template<class From, class To>
concept _unwrap_convertible_to_ = std::convertible_to<unwrap_ref_t<From>, To>;

template<class From, class To, class... Tos>
concept _unwrap_convertible_to_one_of_ = _convertible_to_one_of_<unwrap_ref_t<From>, To, Tos...>;


// std::same_as<T, U> subsumes(includes/implies) std::same_as<U, T> and vice versa.
// If your concept doesn't need such subsumption relation, _same_as_ can be used.
template<class T, class U>
concept _same_as_ = std::is_same_v<T, U>;

template<class T, class U>
concept _no_cvref_same_as_ = std::is_same_v<std::remove_cvref_t<T>, U>;

template<class T, class U, class... Us>
concept _one_of_ = (std::is_same_v<T, U> || ... || std::is_same_v<T, Us>);

template<class T, class U, class... Us>
concept _no_cvref_one_of_ = _one_of_<std::remove_cvref_t<T>, U, Us...>;

template<class T, class U>
concept _unwrap_no_cvref_same_as_ = _same_as_<unwrap_no_cvref_t<T>, U>;

template<class From, class To, class... Tos>
concept _unwrap_no_cvref_one_of_ = _one_of_<unwrap_no_cvref_t<From>, To, Tos...>;


template<class T, class... Args> concept           _constructible_ = std::          is_constructible_v<std::remove_cvref_t<T>, Args...>;
template<class T, class... Args> concept _trivially_constructible_ = std::is_trivially_constructible_v<std::remove_cvref_t<T>, Args...>;
template<class T, class... Args> concept   _nothrow_constructible_ = std::  is_nothrow_constructible_v<std::remove_cvref_t<T>, Args...>;

template<class T> concept           _default_constructible_ = std::          is_default_constructible_v<std::remove_cvref_t<T>>;
template<class T> concept _trivially_default_constructible_ = std::is_trivially_default_constructible_v<std::remove_cvref_t<T>>;
template<class T> concept   _nothrow_default_constructible_ = std::  is_nothrow_default_constructible_v<std::remove_cvref_t<T>>;

template<class T> concept _trivially_copyable_ = std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

template<class T> concept _trivial_ = _trivially_constructible_<T> && _trivially_copyable_<T>;
template<class T> concept _standard_layout_ = std::is_standard_layout_v<std::remove_cvref_t<T>>;

template<class T> concept _pod_   = _trivial_<T> && _standard_layout_<T>;
template<class T> concept _class_ = std::is_class_v<std::remove_cvref_t<T>>;

template<class T> concept _void_     = std::is_void_v<T>;
template<class T> concept _non_void_ = ! std::is_void_v<T>;
template<class T> concept _ref_      = std::is_reference_v<T>;
template<class T> concept _lval_ref_ = std::is_lvalue_reference_v<T>;
template<class T> concept _rval_ref_ = std::is_rvalue_reference_v<T>;
template<class T> concept _lref_     = std::is_lvalue_reference_v<T>;
// template<class T> concept _rref_     = std::is_rvalue_reference_v<T>;
template<class T> concept _const_    = std::is_const_v<T>;
template<class T> concept _volatile_ = std::is_volatile_v<T>;

template<class T> concept _ptr_ = std::is_pointer_v<std::remove_reference_t<T>>;

template<class T> concept _nullptr_ = std::is_null_pointer_v<std::remove_reference_t<T>>;

template<class T> concept _enum_ = std::is_enum_v<std::remove_reference_t<T>>;
template<class T> concept _scoped_enum_ = std::is_scoped_enum_v<std::remove_reference_t<T>>;

template<class T> concept _arithmetic_ = std::    is_arithmetic_v<std::remove_reference_t<T>>;
template<class T> concept _unsigned_   = std::      is_unsigned_v<std::remove_reference_t<T>>; // for unsigned arithmetic
template<class T> concept _signed_     = std::        is_signed_v<std::remove_reference_t<T>>; // for signed arithmetic, also including floating point
template<class T> concept _integral_   = std::      is_integral_v<std::remove_reference_t<T>>; // NOTE: std::integral won't remove reference
template<class T> concept _fp_         = std::is_floating_point_v<std::remove_reference_t<T>>; // NOTE: std::floating_point won't remove reference
template<class T> concept _bool_       = _no_cvref_same_as_<T, bool>;

template<class T> concept _non_bool_integral_ = ! _bool_<T> && _integral_<T>;
template<class T> concept _signed_integral_   = _signed_<T> && _integral_<T>;

template<class T> concept _byte_   = (sizeof(T) == sizeof(std::byte)) && _pod_<T>;
template<class T> concept _8bits_  = (sizeof(T) * CHAR_BIT == 8 ) && _pod_<T>;
template<class T> concept _16bits_ = (sizeof(T) * CHAR_BIT == 16) && _pod_<T>;
template<class T> concept _32bits_ = (sizeof(T) * CHAR_BIT == 32) && _pod_<T>;
template<class T> concept _64bits_ = (sizeof(T) * CHAR_BIT == 64) && _pod_<T>;

template<class T> concept _char_t_      = _no_cvref_one_of_<T, char, wchar_t, char8_t, char16_t, char32_t>; // signed char, unsigned char are excluded, as they are mostly use as int8_t, uint8_t
template<class T> concept _byte_char_t_ = (sizeof(T) == sizeof(std::byte)) && _char_t_<T>;

template<class T> concept _ignore_ = _no_cvref_same_as_<T, ignore_t>;

template<class T> concept _array_ = std::is_array_v<std::remove_reference_t<T>>; // array types are considered to have the same cv-qualification as their element types, https://stackoverflow.com/questions/27770228/what-type-should-stdremove-cv-produce-on-an-array-of-const-t

template<class T, size_t N> void _std_array_impl(std::array<T, N>&);
template<class T> concept _std_array_ = requires(std::remove_cvref_t<T>& t){ dsk::_std_array_impl(t); };
template<class T> concept _array_class_ = _array_<T> && _class_<T>;

template<class T> concept _std_tuple_ = requires(T& t){ std::tuple_size<std::remove_reference_t<T>>::value; };

template<class T, class D> void _unique_ptr_impl(std::unique_ptr<T, D>&);
template<class T         > void _shared_ptr_impl(std::shared_ptr<T   >&);
template<class T> concept _unique_ptr_ = requires(std::remove_cvref_t<T>& t){ dsk::_unique_ptr_impl(t); };
template<class T> concept _shared_ptr_ = requires(std::remove_cvref_t<T>& t){ dsk::_shared_ptr_impl(t); };
template<class T> concept _smart_ptr_  = _unique_ptr_<T> || _shared_ptr_<T>;


inline constexpr struct get_handle_cpo
{
    constexpr auto operator()(auto& t) const noexcept
    requires(
        requires{ dsk_get_handle(*this, t); })
    {
        return dsk_get_handle(*this, t);
    }
}get_handle;

// constexpr auto* dsk_get_handle(get_handle_cpo, auto* h) noexcept { return h; }
constexpr auto* dsk_get_handle(get_handle_cpo, _smart_ptr_ auto& h) noexcept { return h.get(); }

template<class T> concept _handle_ = requires(T& t){ get_handle(t); };

template<class H>
using handle_value_t = decltype(get_handle(std::declval<H&>()));

template<class F, class T>
concept _similar_handle_to_ = std::convertible_to<handle_value_t<F>, handle_value_t<T>>;


template<class T> void _std_optional_impl(std::optional<T>&);
template<class T> concept _std_optional_ = requires(std::remove_cvref_t<T>& t){ dsk::_std_optional_impl(t); };


// unlike is_invocable_r which matches void return type to any type, this only matches void return type to void
template<class R, class F, class... Args>
concept _invocable_r_ = requires(F&& f, Args&&... args){
    {std::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)} -> std::convertible_to<R>;
};


struct callable_helper{ void operator()(){}; };

// detect function/function-pointer and functor/lambda, with any form of template, parameters or return type.
template<class T>
concept _callable_ = std::is_function_v<std::remove_pointer_t<std::remove_cvref_t<T>>>
                     || (std::is_class_v<std::remove_cvref_t<T>>
                         && (requires{ &std::remove_cvref_t<T>::operator(); } // T defines non templated operator()
                             || ! requires{ &inherit_t<std::remove_cvref_t<T>, callable_helper>::operator(); }));
                             //^^^^^^ T defines templated operator(), but as an operator(),
                             //       it's ambiguous with callable_helper's operator().


template<class T>
concept _nullary_callable_ = requires(T t){ t(); };


template<template<class...> class B, class... T>
void _specialized_from_impl(B<T...>&);

// Check if T is a specialization or derived from a specialization of B.
// NOTE: template with non-defaulted non type template parameter is not supported.
template<class T, template<class...> class B>
concept _specialized_from_ = requires(std::remove_cvref_t<T>& t){ dsk::_specialized_from_impl<B>(t); };


template<class T> concept   _blank_ = _no_cvref_same_as_<T,   blank_t>;
template<class T> concept _null_op_ = _no_cvref_same_as_<T, null_op_t>;


inline constexpr struct npos_t{} npos;

constexpr bool operator==(_unsigned_ auto p, npos_t) noexcept { return p == static_cast<decltype(p)>(-1); }
constexpr bool operator!=(_unsigned_ auto p, npos_t) noexcept { return p != static_cast<decltype(p)>(-1); }

constexpr bool operator==(npos_t, _unsigned_ auto p) noexcept { return p == npos; }
constexpr bool operator!=(npos_t, _unsigned_ auto p) noexcept { return p != npos; }


template<class R>
using unchecked_range_value_t = std::iter_value_t<std::ranges::iterator_t<R>>;

template<class R, class V>
concept _range_of_ = std::ranges::range<R>
                     && std::is_same_v<unchecked_range_value_t<R>, V>;

template<class R, class V>
concept _contiguous_range_of_ = std::ranges::contiguous_range<R>
                                && std::is_same_v<unchecked_range_value_t<R>, V>;


constexpr auto* range_data(auto&& r) noexcept { return std::ranges::data(DSK_FORWARD(r)); }
constexpr auto  range_data(null_op_t) noexcept { return nullptr; }

template<class S>
constexpr S range_size(auto&& r) noexcept
{
    return static_cast<S>(std::ranges::size(DSK_FORWARD(r)));
}

template<class R>
concept _range_ref_ = std::ranges::range<R> && std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>;

template<class R>
concept _clearable_range_ = std::ranges::range<R>
                            && (requires(std::remove_cvref_t<R>& r){ r.clear(); }
                                || (std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>
                                    && requires(std::remove_cvref_t<R>& r){ r = std::remove_cvref_t<R>(); }));

void clear_range(_clearable_range_ auto& r)
{
    if constexpr(requires{ r.clear(); }) r.clear();
    else                                 r = DSK_NO_CVREF_T(r)();
}


template<class D, class B>
static constexpr bool is_derived_from_v = std::is_base_of_v<B, D>;

template<class D, class B, class... Bs>
static constexpr bool is_derived_from_one_of_v = (is_derived_from_v<D, B> || ... || is_derived_from_v<D, Bs>);


template<class D, class B>
concept _derived_from_ = is_derived_from_v<std::remove_reference_t<D>, B>;

template<class D, class B, class... Bs>
concept _derived_from_one_of_ = is_derived_from_one_of_v<std::remove_reference_t<D>, B, Bs...>;


// cast to copy_cvref_t<From&&, To>, only allow implicit conversion (i.e. no downcast).
template<class To, class From>
[[nodiscard]] constexpr copy_cvref_t<From&&, To> similar_cast(From&& from) noexcept
requires(                 //v: test lvalue ref, as rvalue ref can bind to prvalue, float&& is convertible to double&&
    std::convertible_to<From&, copy_cvref_t<From&, To>>)
{
    return DSK_FORWARD(from);
}

// cast to copy_cv_t<From, To>*, only allow implicit conversion (i.e. no downcast).
template<class To, class From>
[[nodiscard]] constexpr copy_cv_t<From, To>* similar_cast(From* from) noexcept
requires(
    std::convertible_to<From&, copy_cv_t<From, To>&>)
{
    return from;
}


// cast to copy_cvref_t<From&&, To>, only allow implicit conversion (i.e. no downcast)
template<class To, class From>
[[nodiscard]] constexpr copy_cvref_t<From&&, To> to_similar_ref(From&& from) noexcept
requires(                 //v: test lvalue ref, as rvalue ref can bind to prvalue, float&& is convertible to double&&
    std::convertible_to<From&, copy_cvref_t<From&, To>>)
{
    return DSK_FORWARD(from);
}

// cast to copy_cvref_t<From&, To>, only allow implicit conversion (i.e. no downcast)
template<class To, class From>
[[nodiscard]] constexpr copy_cvref_t<From&, To> to_similar_lref(From& from) noexcept
requires(
    std::convertible_to<From&, copy_cvref_t<From&, To>>)
{
    return from;
}


template<class From, class To>
concept _similar_ref_to_ = _ref_<From> && _ref_<To>
                           && std::convertible_to<From&, To&>
                           && std::same_as<From, copy_cvref_t<To, std::remove_cvref_t<From>>>;

template<class From, class To>
concept _similar_lref_to_ = _lref_<From> && _lref_<To> && _similar_ref_to_<From, To>;


// auto&& v = lref_or_default<T>(t);

template<class T>
constexpr auto lref_or_default(auto& t) noexcept -> decltype(to_similar_lref<T>(t))
{
    return to_similar_lref<T>(t);
}

template<class T>
constexpr T lref_or_default(/*auto&&*/null_op_t) noexcept
{
    return T();
}

// auto&& v = lref_or_default<DefaultValue>(t);

template<auto D, class T = decltype(D)>
constexpr T& lref_or_default(T& t) noexcept
{
    return t;
}

template<auto D, class T = decltype(D)>
constexpr T lref_or_default(/*auto&&*/null_op_t) noexcept
{
    return static_cast<T>(D);
}

// auto&& t = lref_or_default(t, defaultValue);

template<class T>
constexpr T& lref_or_default(T& t, T const&) noexcept
{
    return t;
}

constexpr auto lref_or_default(/*auto&&*/null_op_t, auto&& d) noexcept -> DSK_DECAY_NON_LREF_T(d)
{
    return DSK_FORWARD(d);
}


} // namespace dsk
