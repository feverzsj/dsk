#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <memory>
#include <type_traits>


namespace dsk{


// In Most Cases:
//   for type traits, apply std::remove_cv_t on the tested type;
//   for concepts, apply std::remove_cvref_t on the tested type;


#define DSK_NO_CVREF_T(...) std::remove_cvref_t<decltype(__VA_ARGS__)>
#define DSK_NO_REF_T(...) std::remove_reference_t<decltype(__VA_ARGS__)>
#define DSK_NO_CV_T(...) std::remove_cv_t<decltype(__VA_ARGS__)>


template<class T>
inline constexpr std::type_identity<T> type_c;

#define DSK_TYPE_C(...) ::dsk::type_c<decltype(__VA_ARGS__)>
#define DSK_NO_CVREF_TYPE_C(...) ::dsk::type_c<DSK_NO_CVREF_T(__VA_ARGS__)>


template<bool Cond, class T>
using null_if_t = std::conditional_t<Cond, null_op_t, T>;

template<bool Cond, class T>
using not_null_if_t = null_if_t<! Cond, T>;


// see uelm_t for a example
#define DSK_PARAM(...) __VA_ARGS__


// useful when TrueVal and FalseVal are inconvertible.
#define DSK_CONDITIONAL_V_CAPTURE(Capture, Cond, TrueExp, ...) \
    [Capture]() -> decltype(auto)                              \
    {                                                          \
        if constexpr(Cond) return TrueExp;                     \
        else               return __VA_ARGS__;                 \
    }()                                                        \
/**/

#define DSK_CONDITIONAL_V(Cond, TrueExp, ...) DSK_CONDITIONAL_V_CAPTURE(& , Cond, TrueExp, __VA_ARGS__)
#define DSK_CONDITIONAL_V_NO_CAPTURE(Cond, TrueExp, ...) DSK_CONDITIONAL_V_CAPTURE( , Cond, TrueExp, __VA_ARGS__)


// std::conditional_t but lazily evaulated
#define DSK_CONDITIONAL_T(Cond, TrueType, ...)                \
    typename decltype([]()                                    \
    {                                                         \
        if constexpr(Cond) return ::dsk::type_c<TrueType>;    \
        else               return ::dsk::type_c<__VA_ARGS__>; \
    }())::type                                                \
/**/

#define DSK_DECL_TYPE_IF(Cond, ...)                                 \
    DSK_CONDITIONAL_T(Cond, DSK_PARAM(__VA_ARGS__), ::dsk::blank_t) \
/**/

#define DSK_DEF_MEMBER_IF(Cond, ...)                       \
    DSK_NO_UNIQUE_ADDR DSK_DECL_TYPE_IF(Cond, __VA_ARGS__) \
/**/

#define DSK_DEF_VAR_IF(Cond, ...)                        \
    [[maybe_unused]] DSK_DECL_TYPE_IF(Cond, __VA_ARGS__) \
/**/


template<class T>
using lref_or_val_t = DSK_CONDITIONAL_T(std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>);

#define DSK_LREF_OR_VAL_T(...) ::dsk::lref_or_val_t<decltype(__VA_ARGS__)>
#define DSK_DEF_LREF_OR_VAL(r, ...) DSK_LREF_OR_VAL_T(__VA_ARGS__) r = DSK_FORWARD(__VA_ARGS__)


template<class T>
using lref_or_rref_t = DSK_CONDITIONAL_T(std::is_lvalue_reference_v<T>, T, T&&);

#define DSK_MOVE_NON_LREF(...) static_cast<::dsk::lref_or_rref_t<decltype(__VA_ARGS__)>>(__VA_ARGS__)


template<class T>
using decay_non_lref_t = DSK_CONDITIONAL_T(std::is_lvalue_reference_v<T>, T, std::decay_t<T>);

#define DSK_DECAY_NON_LREF_T(...) ::dsk::decay_non_lref_t<decltype(__VA_ARGS__)>
#define DSK_DECAY_T(...) std::decay_t<decltype(__VA_ARGS__)>


template<template<class...> class T, class... Heads>
constexpr auto make_templated(auto&&... args)
{
    return T<Heads..., DSK_DECAY_T(args)...>(DSK_FORWARD(args)...);
}

template<template<class...> class T, class... Heads>
constexpr auto make_templated_fwd_lref(auto&&... args)
{
    return T<Heads..., DSK_DECAY_NON_LREF_T(args)...>(DSK_FORWARD(args)...);
}

template<template<auto Nt1, class...> class T, auto Nt1, class... Heads>
constexpr auto make_templated_nt1(auto&&... args)
{
    return T<Nt1, Heads..., DSK_DECAY_T(args)...>(DSK_FORWARD(args)...);
}

template<template<auto Nt1, auto Nt2, class...> class T, auto Nt1, auto Nt2, class... Heads>
constexpr auto make_templated_nt2(auto&&... args)
{
    return T<Nt1, Nt2, Heads..., DSK_DECAY_T(args)...>(DSK_FORWARD(args)...);
}


template<class From, class To> using copy_const_t    = DSK_CONDITIONAL_T(std::is_const_v   <From>, std::add_const_t   <To>, To);
template<class From, class To> using copy_volatile_t = std::conditional_t<std::is_volatile_v<From>, std::add_volatile_t<To>, To>;

template<class From, class To> using copy_cv_t = copy_const_t<From, copy_volatile_t<From, To>>;

// NOTE: follow the reference collapse rules:
//       only when From is rval-ref and To is rval-ref or non-ref, the result is rval-ref;
//       all other combinations result lval-ref.
template<class From, class To>
using copy_reference_t = DSK_CONDITIONAL_T(
    std::is_rvalue_reference_v<From>,
    std::add_rvalue_reference_t<To>,
    DSK_CONDITIONAL_T(
        std::is_lvalue_reference_v<From>,
        std::add_lvalue_reference_t<To>,
        To
    )
);

template<class From, class To>
using copy_cvref_t = copy_reference_t<
    From,
    copy_reference_t<
        To,
        copy_cv_t<
            std::remove_reference_t<From>,
            std::remove_reference_t<To>
        >
    >
>;

// NOTE: alias_cast()s don't use c-style cast to avoid static_cast.
//       Don't use it if you want to cast to pointer-interconvertible type.
// ref: https://en.cppreference.com/w/cpp/language/reinterpret_cast#Notes

// NOTE: To may have cv qualifiers but not ref qualifiers
template<class To, class From>
[[nodiscard]] constexpr auto& alias_cast(From& from) noexcept
{
    static_assert(alignof(To) <= alignof(From));
    static_assert( sizeof(To) <=  sizeof(From));

    return reinterpret_cast<copy_cv_t<From, To>&>(from);
}

// NOTE: To may have cv qualifiers but not ref qualifiers
template<class To, class From>
[[nodiscard]] constexpr auto* alias_cast(From* from) noexcept
{
    DSK_ASSERT(std::uintptr_t(from) % alignof(From) == 0);

    static_assert(alignof(To) <= alignof(From));
    static_assert( sizeof(To) <=  sizeof(From));

    return reinterpret_cast<copy_cv_t<From, To>*>(from);
}


// only allow move non const lvalue.
template<class T>
[[nodiscard]] constexpr T&& mut_move(T& t) noexcept
{
    static_assert(! std::is_reference_v<T>);
    static_assert(! std::is_const_v<T>);

    return static_cast<T&&>(t);
}


using std::ignore;
using ignore_t = DSK_NO_CVREF_T(std::ignore);


} // namespace dsk
