#pragma once

#include <cstddef>
#include <cassert>
#include <utility>


#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif


// DSK_REAL_MSVC
#if defined(_MSC_VER) && !defined(__clang__)
    #define DSK_REAL_MSVC
#endif

// DSK_FORCEINLINE
#ifdef DSK_REAL_MSVC
    #define DSK_FORCEINLINE __forceinline
#else
    #define DSK_FORCEINLINE inline __attribute__ ((__always_inline__))
#endif

// DSK_FORCEINLINE_LAMBDA
// usage: []() mutable/constexpr/consteval/static noexcept DSK_FORCEINLINE_LAMBDA trailing-type requires {}
#ifdef DSK_REAL_MSVC
    #define DSK_FORCEINLINE_LAMBDA [[msvc::forceinline]]
#else
    #define DSK_FORCEINLINE_LAMBDA __attribute__((always_inline))
#endif

// DSK_NO_UNIQUE_ADDR
#ifdef _MSC_VER
    #define DSK_NO_UNIQUE_ADDR [[msvc::no_unique_address]]
#else//elif __has_cpp_attribute(no_unique_address)
    #define DSK_NO_UNIQUE_ADDR [[no_unique_address]]
// #else
//     #define DSK_NO_UNIQUE_ADDR
#endif

// DSK_UNREACHABLE
#ifdef DSK_REAL_MSVC
    #define DSK_UNREACHABLE() __assume(false)
#else
    #define DSK_UNREACHABLE() __builtin_unreachable()
#endif

// DSK_LIKELY / DSK_UNLIKELY
#if __has_builtin(__builtin_expect)
    #define DSK_LIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 1)
    #define DSK_UNLIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 0)
#else
    #define DSK_LIKELY(...) __VA_ARGS__
    #define DSK_UNLIKELY(...) __VA_ARGS__
#endif


// DSK_TRAP
// like assert but for release code.
#ifdef DSK_REAL_MSVC
    #define DSK_TRAP() __debugbreak()
#elif __has_builtin(__builtin_debugtrap)
    #define DSK_TRAP() __builtin_debugtrap()
#else
    #define DSK_TRAP() __builtin_trap()
#endif


// NOTE: 'e' should be a reference;
//       for class member access expr, use DSK_FORWARD((e.m/e->m)) or DSK_FORWARD(e).m/->m
#define DSK_FORWARD(e) static_cast<decltype(e)&&>(e)

#define DSK_AUTO_MEMBER(name, ...)                                  \
    static constexpr auto                                           \
        __dsk_auto_member_helper_for_##name= []() -> decltype(auto) \
        {                                                           \
            return __VA_ARGS__;                                     \
        };                                                          \
    decltype(__dsk_auto_member_helper_for_##name())                 \
        name = __dsk_auto_member_helper_for_##name()                \
/**/


// requires expression supports short circuit
#define DSK_REQ_EXP(...) requires{ requires __VA_ARGS__; }

#define DSK_IDX_EXP(...) []<auto I>(){ return __VA_ARGS__; }
#define DSK_TYPE_EXP_OF(T, ...) []<class T>(){ return __VA_ARGS__; }
#define DSK_TYPE_EXP(...) DSK_TYPE_EXP_OF(T, __VA_ARGS__)
#define DSK_TYPE_REQ(...) []<class T>(){ return requires{ requires __VA_ARGS__; }; }
#define DSK_TYPE_IDX_EXP(...) []<class T, auto I>(){ return __VA_ARGS__; }
#define DSK_TYPE_IDX_REQ(...) []<class T, auto I>(){ return requires{ requires __VA_ARGS__; }; }
#define DSK_IDX_LIST_EXP(...) []<auto I, class L>(){ return __VA_ARGS__; }
#define DSK_TYPE_IDX_LIST_EXP(...) []<class T, auto I, class L>(){ return __VA_ARGS__; }

#define DSK_ELM_EXP(...) [](auto&& e) -> decltype(auto) { return __VA_ARGS__; }


// overload 0 and 1 argument macros f0, f1
#define DSK_OVERLOAD_0_1(f0, f1, ...) DSK_OVERLOAD_0_1_IMPL(_0 __VA_OPT__(,) __VA_ARGS__, f1, f0)(__VA_ARGS__)
#define DSK_OVERLOAD_0_1_IMPL(_0, _1, NAME, ...) NAME


// Joins the two arguments together, even when argument is itself a macro (see 16.3.1 in C++ standard).
// The key is that macro expansion of macro arguments doesn't occur in DSK_JOIN_IMPL2 but does in DSK_JOIN_IMPL.
#define DSK_JOIN(X, Y) DSK_JOIN_IMPL(X, Y)
#define DSK_JOIN_IMPL(X, Y) DSK_JOIN_IMPL2(X,Y)
#define DSK_JOIN_IMPL2(X, Y) X##Y


namespace dsk{


using size_t    = std::size_t;
using ptrdiff_t = std::ptrdiff_t;

using std::in_place_t;
using std::in_place;


constexpr bool is_oneof(auto const& v, auto const&... es) noexcept
{
    return (... || (v == es));
}

constexpr bool test_flag(auto v, auto f) noexcept
{
    return (v & f) != 0;
}

constexpr bool in_bound(auto const& v, auto const& min, auto const& max) noexcept
{
    return min <= v && v < max;
}

constexpr bool in_bound(auto const& v, auto const& max) noexcept
{
    return in_bound(v, 0, max);
}


template<auto V>
struct constant
{
    static constexpr auto value = V;

    using value_type = decltype(V);
    constexpr operator value_type() const noexcept { return value; }
};

template<auto V>
inline constexpr auto constant_c = constant<V>();


template<auto... V>
struct constant_seq
{
    static constexpr size_t size() noexcept
    {
        return sizeof...(V);
    }
};


template<class T>
struct empty_range_t
{
    constexpr T*    begin() const noexcept { return nullptr; }
    constexpr T*      end() const noexcept { return nullptr; }
    constexpr T*     data() const noexcept { return nullptr; }
    constexpr size_t size() const noexcept { return 0; }
    constexpr bool  empty() const noexcept { return true; }
    constexpr void operator[](size_t) const noexcept {} // necessary for some constraint check
};

template<class T>
constexpr auto empty_range = empty_range_t<T>{};


template<bool Cond>
constexpr auto conditional_ret(auto if_, auto else_) noexcept
{
    if constexpr(Cond) return if_;
    else               return else_;
}


struct empty_t{};

struct blank_t
{
    constexpr blank_t(auto&&...) noexcept {}
};

inline constexpr blank_t blank_c;


struct null_op_t
{
    constexpr null_op_t(auto&&...) noexcept {}
    constexpr void operator()(auto&&...) const noexcept{}
};

inline constexpr null_op_t null_op;


template<class... B>
struct inherit_t : B... {};


// https://stackoverflow.com/a/40873657
// NOTE: F must define return type explicitly
template<class F>
struct y_combinator
{
    F f;

    constexpr decltype(auto) operator()(auto&&... args) const
    noexcept(noexcept(
        f(*this, DSK_FORWARD(args)...)
    ))
    {
        return f(*this, DSK_FORWARD(args)...);
    }

    constexpr decltype(auto) operator()(auto&&... args)
    noexcept(noexcept(
        f(*this, DSK_FORWARD(args)...)
    ))
    {
        return f(*this, DSK_FORWARD(args)...);
    }
};

template<class F>
y_combinator(F) -> y_combinator<F>;


constexpr auto* to_ptr(auto& t) noexcept { return &t; }
constexpr auto* to_ptr(auto* t) noexcept { return t; }
// constexpr auto to_ptr(std::nullptr_t) noexcept { return nullptr; }
constexpr auto to_ptr(null_op_t) noexcept { return nullptr; }


enum void_result_e
{
    void_as_void,
    voidness_as_void
};

struct voidness{};

template<void_result_e VE>
auto return_void() noexcept
{
         if constexpr(VE ==     void_as_void) return void    ();
    else if constexpr(VE == voidness_as_void) return voidness();
    else
        static_assert(false, "unsupported value");
}


template<class... Ts>
constexpr decltype(auto) may_invoke_with_ts(auto&& f, auto&&... args)
{
    if constexpr(requires{ f.template operator()<Ts...>(DSK_FORWARD(args)...); })
    {
        return f.template operator()<Ts...>(DSK_FORWARD(args)...);
    }
    else
    {
        return f(DSK_FORWARD(args)...);
    }
}

template<auto... Nts>
constexpr decltype(auto) may_invoke_with_nts(auto&& f, auto&&... args)
{
    if constexpr(requires{ f.template operator()<Nts...>(DSK_FORWARD(args)...); })
    {
        return f.template operator()<Nts...>(DSK_FORWARD(args)...);
    }
    else
    {
        return f(DSK_FORWARD(args)...);
    }
}


enum start_scheduler_e
{
    dont_start_now = false,
    start_now = true,
};


} // namespace dsk
