#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/isclose.hpp>
#include <dsk/util/concepts.hpp>
#include <limits>


namespace dsk
{


// If all values of S can be represented by T.
// Suppose IEEE 754 floating point types.
// https://en.cppreference.com/w/cpp/language/list_initialization#Narrowing_conversions
// https://stackoverflow.com/questions/3793838/which-is-the-first-integer-that-an-ieee-754-float-is-incapable-of-representing-e
template<class S, class T>
concept _lossless_convertible_ = requires(S s){ std::remove_cvref_t<T>{s}; } // Non narrowing conversions, fp -> wider fp, integral/unscoped enum -> wider integral
                                 || (_integral_<S> && _fp_<T> && sizeof(S) < sizeof(T))
                                 || (_no_cvref_same_as_<S, bool> || _no_cvref_same_as_<T, bool>);

template<class T, class S>
concept _lossless_convertible_to_ = _lossless_convertible_<S, T>;


enum num_cast_err_report_e
{
    num_cast_return_err,             // always return expected<T, errc> or errc.
    num_cast_return_err_for_losable, // only return expected<T, errc> or errc if ! _lossless_convertible_<S, T>.
    num_cast_throw_err               // throw system_error.
};


template<
    _arithmetic_ T,
    close_options Opts = {},
    num_cast_err_report_e ErrReport = num_cast_return_err,
    _arithmetic_ S
>
constexpr auto num_cast(S s) noexcept
->
    std::conditional_t<ErrReport == num_cast_return_err, expected<T, errc>, T>
requires(
    _lossless_convertible_<S, T>)
{
    return static_cast<T>(s);
}

template<
    _arithmetic_ T,
    close_options Opts = {},
    num_cast_err_report_e ErrReport = num_cast_return_err,
    _arithmetic_ S
>
constexpr auto num_cast(S s) noexcept(ErrReport != num_cast_throw_err)
->
    std::conditional_t<ErrReport != num_cast_throw_err, expected<T, errc>, T>
{
    constexpr bool fpS = std::is_floating_point_v<S>;
    constexpr bool fpT = std::is_floating_point_v<T>;
    constexpr auto tLower = std::numeric_limits<T>::lowest();
    constexpr auto tUpper = std::numeric_limits<T>::max();

    if constexpr(fpS && fpT) // fpS -> fpT
    {
        static_assert(sizeof(S) > sizeof(T));

        if(isclose_to<Opts>(static_cast<T>(s), s))
        {
            return static_cast<T>(s);
        }
    }
    else if constexpr(fpS && ! fpT) // fpS -> integralT
    {
        if(S rs = std::round(s);
           isclose_to<Opts>(rs, s)
           && tLower <= static_cast<T>(rs) && static_cast<T>(rs) <= tUpper)
        {
            return static_cast<T>(rs);
        }
    }
    else if constexpr(! fpS && fpT) // integralS -> fpT
    {
        static_assert(sizeof(S) >= sizeof(T));

        if(static_cast<S>(static_cast<T>(s)) == s)
        {
            return static_cast<T>(s);
        }
    }
    else // integralS -> integralT
    {
        constexpr bool signedS = std::is_signed_v<S>;
        constexpr bool signedT = std::is_signed_v<T>;

        if constexpr(signedS && signedT) // signedS -> signedT
        {
            static_assert(sizeof(S) > sizeof(T));

            if(static_cast<S>(tLower) <= s && s <= static_cast<S>(tUpper))
            {
                return static_cast<T>(s);
            }
        }
        else if constexpr(signedS) // signedS -> unsignedT
        {
            using w_t = std::conditional_t<(sizeof(S) > sizeof(T)), S, T>; // when same size, choose the unsigned one.

            if(0 <= s && static_cast<w_t>(s) <= static_cast<w_t>(tUpper))
            {
                return static_cast<T>(s);
            }
        }
        else // unsignedS -> signedT / unsignedT
        {
            using w_t = std::conditional_t<(sizeof(S) < sizeof(T)), T, S>; // when same size, choose the unsigned one.

            if(static_cast<w_t>(s) <= static_cast<w_t>(tUpper))
            {
                return static_cast<T>(s);
            }
        }
    }

    if constexpr(ErrReport == num_cast_throw_err) throw system_error(errc::bad_num_cast);
    else                                          return errc::bad_num_cast;
}



template<
    close_options Opts = {},
    num_cast_err_report_e ErrReport = num_cast_return_err,
    _arithmetic_ S,
    _arithmetic_ T
>
constexpr auto cvt_num(S s, T& t) noexcept requires(_lossless_convertible_<S, T>)
{
    t = static_cast<T>(s);

    if constexpr(ErrReport == num_cast_return_err)
    {
        return errc();
    }
}

template<
    close_options Opts = {},
    num_cast_err_report_e ErrReport = num_cast_return_err,
    _arithmetic_ S,
    _arithmetic_ T
>
constexpr auto cvt_num(S s, T& t) noexcept(ErrReport != num_cast_throw_err)
{
    if constexpr(ErrReport == num_cast_throw_err)
    {
        t = num_cast<T, Opts, ErrReport>(s);
    }
    else
    {
        auto r = num_cast<T, Opts, ErrReport>(s);

        if(! has_err(r))
        {
            t = get_val(r);
            return errc();
        }

        return get_err(r);
    }
}


// return T if _lossless_convertible_<S, T>,
// otherwise return expected<T, errc>
template<
    _arithmetic_ T,
    close_options Opts = {}
>
constexpr auto num_cast_u(_arithmetic_ auto s) noexcept
{
    return num_cast<T, Opts, num_cast_return_err_for_losable>(s);
}

// return void if _lossless_convertible_<S, T>,
// otherwise return errc
template<
    close_options Opts = {}
>
constexpr auto cvt_num_u(_arithmetic_ auto s, _arithmetic_ auto& t) noexcept
{
    return cvt_num<Opts, num_cast_return_err_for_losable>(s, t);
}


// throw on err
template<
    _arithmetic_ T,
    close_options Opts = {}
>
constexpr T num_cast_x(_arithmetic_ auto s)
{
    return num_cast<T, Opts, num_cast_throw_err>(s);
}

// throw on err
template<
    close_options Opts = {}
>
constexpr void cvt_num_x(_arithmetic_ auto s, _arithmetic_ auto& t)
{
    cvt_num<Opts, num_cast_throw_err>(s, t);
}


} // namespace dsk
