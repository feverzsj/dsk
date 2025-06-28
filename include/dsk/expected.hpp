#pragma once

#include <dsk/err.hpp>
#include <dsk/optional.hpp>
#include <boost/system/result.hpp>
#include <expected>
#include <functional>


namespace dsk{


using std::unexpected;
using std::unexpect_t;
using std::unexpect;
using std::bad_expected_access;

inline constexpr struct expect_t{} expect;


template<class T = void, class E = error_code>
class [[nodiscard]] expected;


template<class T, class E> void _expected_impl(expected<T, E>&);
template<class T, class E> void _expected_impl(std::expected<T, E>&);
template<class T> concept _expected_ = requires(std::remove_cvref_t<T>& t){ dsk::_expected_impl(t); };

template<class E> void _unexpected_impl(unexpected<E>&);
template<class T> concept _unexpected_ = requires(std::remove_cvref_t<T>& t){ dsk::_unexpected_impl(t); };


template<class T, class E>
class expected : public std::expected<T, E>
{
    using base = std::expected<T, E>;

    constexpr void check_errorable()
    {
    #ifndef NDEBUG
        if constexpr(_errorable_<E>)
        {
            DSK_ASSERT(base::has_value() || has_err(base::error()));
        }
    #endif
    }

public:
    constexpr expected() = default;


    template<class EU>
        requires(_expected_<EU> || _unexpected_<EU>)
    constexpr
        explicit(
            ! std::is_convertible_v<EU, base>
        )
    expected(EU&& r)
        noexcept(
            noexcept(base(DSK_FORWARD(r)))) // this will also apply base ctor's constraints.
        :
        base(DSK_FORWARD(r))
    {
        check_errorable();
    }


    constexpr expected(std::convertible_to<T> auto&& v)
        noexcept(
            noexcept(base(in_place, DSK_FORWARD(v))))
        requires(
            ! std::is_convertible_v<decltype(v), E>)
        :
        base(in_place, DSK_FORWARD(v))
    {}

    constexpr expected(std::convertible_to<E> auto&& e)
        noexcept(
            noexcept(base(unexpect, DSK_FORWARD(e))))
        :
        base(unexpect, DSK_FORWARD(e))
    {
        check_errorable();
    }

    constexpr expected(expect_t, auto&&... args)
        noexcept(
            noexcept(base(in_place, DSK_FORWARD(args)...)))
        //requires(
        //   sizeof...(args) >= 1)
        :
        base(in_place, DSK_FORWARD(args)...)
    {}

    constexpr expected(unexpect_t, auto&&... args)
        noexcept(
            noexcept(base(unexpect, DSK_FORWARD(args)...)))
        //requires(
        //    sizeof...(args) >= 1)
        :
        base(unexpect, DSK_FORWARD(args)...)
    {
        check_errorable();
    }


//     constexpr expected(auto&&... args)
//         noexcept(
//             noexcept(base(in_place, DSK_FORWARD(args)...)))
//         requires(
//             sizeof...(args) >= 2)
//         :
//         base(in_place, DSK_FORWARD(args)...)
//     {}
// 
//     constexpr expected(auto&&... args)
//         noexcept(
//             noexcept(base(unexpect, DSK_FORWARD(args)...)))
//         requires(
//             sizeof...(args) >= 2)
//         :
//         base(unexpect, DSK_FORWARD(args)...)
//     {
//         check_errorable();
//     }

#ifdef _MSC_VER
    /// workaround for unconstrained operator==(std::expected, T2 const&)
    //  TODO: remove when constrained
    
    friend constexpr bool operator==(expected const& l, expected const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) == static_cast<base const&>(r); })
    {
        return static_cast<base const&>(l) == static_cast<base const&>(r);
    }

    template<class T2, class E2>
    friend constexpr bool operator==(expected const& l, std::expected<T2, E2> const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) == r; })
    {
        return static_cast<base const&>(l) == r;
    }

    template<class T2, class E2>
    friend constexpr bool operator==(std::expected<T2, E2> const& l, expected const& r) noexcept
    requires(requires{
        l == static_cast<base const&>(r); })
    {
        return l == static_cast<base const&>(r);
    }
#endif
};


template<class T, class E>
class expected<T&, E> : public expected<std::reference_wrapper<T>, E>
{
    using base = expected<std::reference_wrapper<T>, E>;

public:
    using value_type = T&;

    using base::base;

    // always return T&
    constexpr T& value(this auto&& s) { return DSK_FORWARD(s).value().get(); }
    constexpr T& operator* () const noexcept { return base::operator*().get(); }
    constexpr T* operator->() const noexcept { return std::addressof(**this); }
};


template<_void_ T, _errorable_ E>
class expected<T, E>
{
    E _e{};

    template<class U, class G>
    friend class expected;

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    constexpr expected() noexcept = default;

    constexpr expected(std::convertible_to<E> auto&& e) noexcept
        : _e(DSK_FORWARD(e))
    {
        DSK_ASSERT(_e);
    }

    template<_void_ U, std::convertible_to<E> G>
    constexpr expected(expected<U, G> const& r) noexcept
        : _e(r._e)
    {}

    template<std::convertible_to<E> G>
    constexpr expected(unexpected<G> const& r) noexcept
        : _e(r.error())
    {
        DSK_ASSERT(_e);
    }

    constexpr expected(expect_t) noexcept
        : _e()
    {}

    constexpr expected(unexpect_t, E const& e) noexcept
        : _e(e)
    {
        DSK_ASSERT(_e);
    }

    constexpr bool has_value() const noexcept { return ! has_err(_e); }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr void operator*() const noexcept { DSK_ASSERT(has_value()); }

    constexpr void value(this auto&& s)
    {
        if(has_err(s._e))
        {
            throw bad_expected_access(DSK_FORWARD(s)._e);
        }
    }

    constexpr auto&& error(this auto&& s) noexcept { return DSK_FORWARD(s)._e; }
    
    // REMOVED: as these makes _error_enum_<expected> true
    //constexpr operator E&&() && noexcept { return mut_move(_e); }
    //constexpr operator E const&() const& noexcept { return _e; }
    //constexpr operator E const&&() const&& noexcept { return std::move(_e); }

    constexpr void emplace() noexcept { _e = E{}; }

    constexpr auto operator<=>(expected const&) const noexcept = default;
};


static_assert(sizeof(expected<void, errc>) == sizeof(errc));
static_assert(sizeof(expected<void, error_code>) == sizeof(error_code));
static_assert(std::is_trivially_copyable_v<expected<void, errc>>);
static_assert(std::is_trivially_copyable_v<expected<void, error_code>>);
static_assert(std::is_convertible_v<errc, expected<void, errc>>);
static_assert(std::is_convertible_v<errc, expected<void, error_code>>);
static_assert(std::is_convertible_v<expected<void, errc>, expected<void, error_code>>);
static_assert(std::is_convertible_v<unexpected<errc>, expected<void, errc>>);
static_assert(std::is_convertible_v<unexpected<errc>, expected<void, error_code>>);
static_assert(std::is_convertible_v<int, expected<int, errc>>);
static_assert(std::is_convertible_v<errc, expected<int, errc>>);
static_assert(std::is_convertible_v<int&, expected<int&, errc>>);
static_assert(std::is_convertible_v<int const&, expected<int const&, errc>>);
static_assert(std::is_convertible_v<int&, expected<int const&, errc>>);
static_assert(std::is_convertible_v<errc, expected<int&, errc>>);
static_assert(std::is_convertible_v<errc, expected<int const&, errc>>);


template<class T = void>
constexpr auto make_expected_if_no(_errorable_ auto&& err, auto&&... args) noexcept -> expected<T, err_t<decltype(err)>>
{
    if(! has_err(err)) return {expect, DSK_FORWARD(args)...};
    else               return {unexpect, get_err(DSK_FORWARD(err))};
}

constexpr auto make_expected_if_no(_errorable_ auto&& err, auto&& v) noexcept -> expected<DSK_DECAY_T(v), err_t<decltype(err)>>
{
    if(! has_err(err)) return {expect, DSK_FORWARD(v)};
    else               return {unexpect, get_err(DSK_FORWARD(err))};
}


constexpr auto gen_expected_if_no(_errorable_ auto&& err, auto&& gen) -> expected<decltype(DSK_FORWARD(gen)()), err_t<decltype(err)>>
{
    if(! has_err(err)) return {expect, DSK_FORWARD(gen)()};
    else               return {unexpect, get_err(DSK_FORWARD(err))};
}


struct expected_like_cpo{};
template<class T> concept _expected_like_ = requires(std::remove_cvref_t<T>& t){ dsk_expected_like(expected_like_cpo(), t); }
                                            || _expected_<T>;


template<class T, class E> void dsk_expected_like(expected_like_cpo, boost::system::result<T, E>&);


constexpr bool dsk_has_err(has_err_cpo, _expected_like_ auto const& t) noexcept
{
    return ! t.has_value();
}

constexpr decltype(auto) dsk_get_err(get_err_cpo, _expected_like_ auto&& t) noexcept
{
    return DSK_FORWARD(t).error();
}

constexpr decltype(auto) dsk_get_val(get_val_cpo, _expected_like_ auto&& t) noexcept
{
    return *DSK_FORWARD(t);
}


template<class T> concept _result_ = requires(T t){ has_err(t); get_err(t); get_val(t); };
template<class T, class V> concept _result_of_ = _same_as_<val_t<T>, V> && _result_<T>;


} // namespace dsk


#define DSK_TRY_UNIQUE_NAME DSK_JOIN(_dsk_try_unique_name_, __COUNTER__)


#define DSK_E_TRY_ONLY_IMPL(ErrLikely, R, ...)             \
    if(auto&& R = __VA_ARGS__; ErrLikely(dsk::has_err(R))) \
        return dsk::get_err(DSK_FORWARD(R))                \
/**/

#define DSK_E_TRY_IMPL(ErrLikely, R, V, ...) \
    auto&& R = __VA_ARGS__;                  \
                                             \
    if(ErrLikely(dsk::has_err(R)))           \
        return dsk::get_err(DSK_FORWARD(R)); \
                                             \
    V = dsk::get_val(DSK_FORWARD(R))         \
/**/

#define DSK_E_TRY_RETURN_IMPL(ErrLikely, R, ...) \
    auto&& R = __VA_ARGS__;                      \
                                                 \
    if(ErrLikely(dsk::has_err(R)))               \
        return dsk::get_err(DSK_FORWARD(R));     \
                                                 \
    return dsk::get_val(DSK_FORWARD(R))          \
/**/


// DSK_E_TRY_ONLY(_errorable_); // value ignored if any
// the caller's return type should be convertible from err_t<_errorable_>.
#define             DSK_E_TRY_ONLY(...) DSK_E_TRY_ONLY_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)
#define DSK_E_TRY_ONLY_LIKELY_FAIL(...) DSK_E_TRY_ONLY_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)

//                     Perfectly forwarded value ref from expected will be assigned to v,
//           vvvvvvvv: so user can use any suitable type specifier or existing variable.
// DSK_E_TRY(auto&& v, _result_of_<NonVoid>);
// V v;
// DSK_E_TRY(v, _result_of_<NonVoid>);
// the caller's return type should be convertible from both err_t<_result_> and val_t<_result_>.
#define              DSK_E_TRY(V, ...)  DSK_E_TRY_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, DSK_PARAM(V), __VA_ARGS__)
#define  DSK_E_TRY_LIKELY_FAIL(V, ...)  DSK_E_TRY_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, DSK_PARAM(V), __VA_ARGS__)

// DSK_E_TRY_FWD(v, _result_of_<NonVoid>);
#define                 DSK_E_TRY_FWD(V, ...)              DSK_E_TRY(DSK_PARAM(auto&& V), __VA_ARGS__)
#define DSK_E_TRY_FORWARD_LIKELY_FAIL(V, ...)  DSK_E_TRY_LIKELY_FAIL(DSK_PARAM(auto&& V), __VA_ARGS__)

// DSK_E_TRY_RETURN(_result_of_<NonVoid>); // useful, when caller's return type is not convertible from _result_.
#define             DSK_E_TRY_RETURN(...)  DSK_E_TRY_RETURN_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)
#define DSK_E_TRY_RETURN_LIKELY_FAIL(...)  DSK_E_TRY_RETURN_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)


/// Universal Try for _result_, _errorable_ or average type.

#define DSK_U_TRY_ONLY_IMPL(ErrLikely, R, ...)             \
    if constexpr(dsk::_errorable_<decltype(__VA_ARGS__)>)  \
    {                                                      \
        DSK_E_TRY_ONLY_IMPL(ErrLikely, R, __VA_ARGS__);    \
    }                                                      \
    else                                                   \
    {                                                      \
        static_assert(dsk::_void_<decltype(__VA_ARGS__)>); \
        __VA_ARGS__;                                       \
    }                                                      \
/**/

#define DSK_U_TRY_IMPL(ErrLikely, R, V, ...)                          \
    auto&& R = __VA_ARGS__;                                           \
                                                                      \
    if constexpr(dsk::_result_<decltype(R)>)                          \
    {                                                                 \
        if(ErrLikely(dsk::has_err(R)))                                \
            return dsk::get_err(DSK_FORWARD(R));                      \
    }                                                                 \
                                                                      \
    V = DSK_CONDITIONAL_V(                                            \
            dsk::_result_<decltype(R)>, dsk::get_val(DSK_FORWARD(R)), \
                                        DSK_FORWARD(R))               \
/**/

#define DSK_U_TRY_RETURN_IMPL(ErrLikely, R, ...)          \
    if constexpr(dsk::_result_<decltype(__VA_ARGS__)>)    \
    {                                                     \
        DSK_E_TRY_RETURN_IMPL(ErrLikely, R, __VA_ARGS__); \
    }                                                     \
    else                                                  \
    {                                                     \
        return __VA_ARGS__;                               \
    }                                                     \
/**/



// DSK_U_TRY_ONLY(_errorable_/_void_); value ignored if any
#define             DSK_U_TRY_ONLY(...) DSK_U_TRY_ONLY_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)
#define DSK_U_TRY_ONLY_LIKELY_FAIL(...) DSK_U_TRY_ONLY_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)

// DSK_U_TRY([decl] v, _result_of_<NonVoid>/NonVoid);
#define              DSK_U_TRY(V, ...)  DSK_U_TRY_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, DSK_PARAM(V), __VA_ARGS__)
#define  DSK_U_TRY_LIKELY_FAIL(V, ...)  DSK_U_TRY_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, DSK_PARAM(V), __VA_ARGS__)

// DSK_U_TRY_FWD(v, _result_of_<NonVoid>/NonVoid);
#define             DSK_U_TRY_FWD(V, ...)             DSK_U_TRY(DSK_PARAM(auto&& V), __VA_ARGS__)
#define DSK_U_TRY_FWD_LIKELY_FAIL(V, ...) DSK_U_TRY_LIKELY_FAIL(DSK_PARAM(auto&& V), __VA_ARGS__)

// DSK_U_TRY_RETURN(_result_of_<NonVoid>/NonVoid);
#define             DSK_U_TRY_RETURN(...)  DSK_U_TRY_RETURN_IMPL(DSK_UNLIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)
#define DSK_U_TRY_RETURN_LIKELY_FAIL(...)  DSK_U_TRY_RETURN_IMPL(  DSK_LIKELY, DSK_TRY_UNIQUE_NAME, __VA_ARGS__)
