#pragma once

#include <dsk/util/str.hpp>
#include <dsk/util/tuple.hpp>
#include <boost/core/detail/string_view.hpp>
#include <array>
#include <limits>
#include <utility>
#include <charconv>
#include <cstring>


namespace dsk{


template<class V, class... Args>
struct with_args : tuple<V, Args...>
{
    constexpr explicit with_args(auto&& v, auto&&... args)
        : tuple<V, Args...>(DSK_FORWARD(v), DSK_FORWARD(args)...)
    {}
};

with_args(auto... vas) -> with_args<decltype(vas)...>;


constexpr auto make_with_args_gen(auto&&... args)
{
    return [...args = DSK_FORWARD(args)](auto&& v)
    {
        return with_args(DSK_FORWARD(v), args...);
    };
}


inline constexpr struct stringify_cpo
{
    constexpr decltype(auto) operator()(auto&& v, auto&&... args) const
    requires
        requires{ dsk_stringify(*this, DSK_FORWARD(v), DSK_FORWARD(args)...); }
    {
        return dsk_stringify(*this, DSK_FORWARD(v), DSK_FORWARD(args)...);
    }

    template<class... VAs>
    constexpr decltype(auto) operator()(with_args<VAs...> const& w) const
    requires
        requires(VAs&&... vas){ dsk_stringify(*this, DSK_FORWARD(vas)...); }
    {
        return apply_elms(w, [&](auto&&... vas) -> decltype(auto)
        {
            return dsk_stringify(*this, DSK_FORWARD(vas)...);
        });
    }
} stringify;


template<class T>
concept _stringifible_ = requires(T&& v){ dsk::stringify(DSK_FORWARD(v)); };


template<bool Cond>
constexpr decltype(auto) stringify_if(_stringifible_ auto&& v)
{
    if constexpr(Cond) return stringify(DSK_FORWARD(v));
    else               return empty_range<char>;
}

template<bool Cond>
constexpr decltype(auto) stringify_if(_stringifible_ auto&& v, _stringifible_ auto&& else_)
{
    if constexpr(Cond) return stringify(DSK_FORWARD(v));
    else               return stringify(DSK_FORWARD(else_));
}


template<class T>
constexpr T* append_as_str(T* d, _stringifible_ auto&&... ss)
{
    return append_str(d, stringify(DSK_FORWARD(ss))...);
}

constexpr void append_as_str(_resizable_buf_ auto& d, _stringifible_ auto&&... ss)
{
    append_str(d, stringify(DSK_FORWARD(ss))...);
}

constexpr void assign_as_str(_resizable_buf_ auto& d, _stringifible_ auto&& ss)
{
    assign_str(d, stringify(DSK_FORWARD(ss)));
}

template<_resizable_buf_ D = string>
constexpr D cat_as_str(_stringifible_ auto&&... ss)
{
    D d;

    if constexpr(sizeof...(ss) == 1) assign_as_str(d, DSK_FORWARD(ss)...);
    else                             append_as_str(d, DSK_FORWARD(ss)...);

    return d;
}

template<_resizable_buf_ D = string>
constexpr decltype(auto) cat_as_or_fwd_str(_stringifible_ auto&&... ss)
{
         if constexpr(sizeof...(ss) == 0) return empty_range<char>;
    else if constexpr(sizeof...(ss) == 1) return stringify(DSK_FORWARD(ss)...);
    else                                  return cat_as_str<D>(DSK_FORWARD(ss)...);
}



template<unsigned Cap>
struct fixed_storage_str
{
    unsigned n = 0;
    char     d[Cap];

    template<unsigned N>
    constexpr void assign_literal(char const (&s)[N]) noexcept
    {
        static_assert(N - 1 <= Cap);
        std::memcpy(d, s, N - 1);
        n = N - 1;
    }

    constexpr void set_end(char const* e) noexcept
    {
        DSK_ASSERT(d <= e && e < d + Cap);
        n = static_cast<unsigned>(e - d);
    }

    static constexpr auto capacity() noexcept { return Cap; }
    constexpr auto size() const noexcept { return n; }

    constexpr char      * data()       noexcept { return d; }
    constexpr char const* data() const noexcept { return d; }

    constexpr char const* begin() const noexcept { return d    ; }
    constexpr char const*   end() const noexcept { return d + n; }
    constexpr char      * begin()       noexcept { return d    ; }
    constexpr char      *   end()       noexcept { return d + n; }

    constexpr char * buf_begin() noexcept { return d      ; }
    constexpr char *   buf_end() noexcept { return d + Cap; }

    constexpr operator std::string_view() const noexcept { return {data(), size()}; }
    constexpr operator boost::core::string_view() const noexcept { return {data(), size()}; }
};


struct radix_t{ unsigned value; };

template<size_t N> struct radix_constant : constant<N> {};
template<size_t N> inline constexpr radix_constant<N> radix_c;

template<size_t N>
constexpr auto with_radix(_non_bool_integral_ auto v) noexcept
{
    return with_args(v, radix_c<N>);
}

constexpr auto with_radix(_non_bool_integral_ auto v, unsigned r) noexcept
{
    return with_args(v, radix_t{r});
}


template<_non_bool_integral_ V>
constexpr size_t stringify_integer_max_bytes(size_t base = 10) noexcept
{
    DSK_ASSERT(2 <= base && base <= 36); // limit of std::to_chars

    constexpr size_t sign = std::signed_integral<V>;
    constexpr size_t sv = sizeof(V);

    if(base < 8) // use base 2
    {
        return sv * 8 + sign; // 2 = 2^1
    }
    else if(base < 16) // use base 8
    {
        return sv * 8 / 3 + 1 + sign; // 8 = 2^3
    }
    else if(base < 10) // use base 16
    {
        return sv * 8 / 4 + sign; // 16 = 2^4
    }
    else // Base >= 10, use base 10
    {
             if constexpr(sv ==  1) return 3 + sign;
        else if constexpr(sv ==  2) return 5 + sign;
        else if constexpr(sv ==  4) return 10 + sign;
        else if constexpr(sv ==  8) return 20 + sign;
        else if constexpr(sv == 16) return 39 + sign;
        else
            static_assert(false, "unsupported type");
    }
}


template<size_t Base = 10>
constexpr auto dsk_stringify(stringify_cpo, _non_bool_integral_ auto v, radix_constant<Base> = {}) noexcept requires(! _char_t_<decltype(v)>)
{
    fixed_storage_str<stringify_integer_max_bytes<decltype(v)>(Base)> b;

    auto[e, ec] = std::to_chars(b.buf_begin(), b.buf_end(), v, Base);
    
    DSK_ASSERT(ec == std::errc());
    
    b.set_end(e);

    return b;
}

constexpr auto dsk_stringify(stringify_cpo, _non_bool_integral_ auto v, radix_t base) noexcept requires(! _char_t_<decltype(v)>)
{
    DSK_ASSERT(2 <= base.value && base.value <= 36);

    fixed_storage_str<stringify_integer_max_bytes<decltype(v)>(2)> b;

    auto[e, ec] = std::to_chars(b.buf_begin(), b.buf_end(), v, static_cast<int>(base.value));

    DSK_ASSERT(ec == std::errc());

    b.set_end(e);

    return b;
}


struct prec_t{ unsigned value; };

template<size_t N> struct prec_constant : constant<N> {};
template<size_t N> inline constexpr prec_constant<N> prec_c;

using fp_fmt = std::chars_format;

template<fp_fmt F> struct fp_fmt_constant : constant<F> {};
template<fp_fmt F> inline constexpr fp_fmt_constant<F> fp_fmt_c;


template<size_t P>
constexpr auto with_prec(std::floating_point auto v) noexcept
{
    return with_args(v, prec_c<P>);
}

constexpr auto with_prec(std::floating_point auto v, unsigned p) noexcept
{
    return with_args(v, prec_t{p});
}

template<fp_fmt F, size_t P = size_t(-1)>
constexpr auto with_fmt(std::floating_point auto v) noexcept
{
    return with_args(v, fp_fmt_c<F>, prec_c<P>);
}

template<char F, size_t P = size_t(-1)>
constexpr auto with_fmt(std::floating_point auto v) noexcept
{
         if constexpr(F == 'f') return with_fmt<fp_fmt::fixed     , P>(v);
    else if constexpr(F == 'e') return with_fmt<fp_fmt::scientific, P>(v);
    else if constexpr(F == 'a') return with_fmt<fp_fmt::hex       , P>(v);
    else if constexpr(F == 'g') return with_fmt<fp_fmt::general   , P>(v);
    else
        static_assert(false, "invalid format");
}

constexpr auto with_fmt(std::floating_point auto v, fp_fmt f) noexcept
{
    return with_args(v, f);
}

constexpr auto with_fmt(std::floating_point auto v, fp_fmt f, unsigned p) noexcept
{
    return with_args(v, f, prec_t{p});
}


consteval size_t log10ceil(size_t num) noexcept
{
    return (num < 10) ? 1 : (1 + log10ceil(num / 10));
}

// https://stackoverflow.com/questions/68472720/stdto-chars-minimal-floating-point-buffer-size
// https://www.reddit.com/r/cpp/comments/dgj89g/cppcon_2019_stephan_t_lavavej_floatingpoint/f3ixjs2/
template<std::floating_point V>
constexpr size_t stringify_fp_max_bytes(fp_fmt fmt = {}, size_t prec = size_t(-1)) noexcept
{
    constexpr size_t dig2s       = std::numeric_limits<V>::digits;
    constexpr size_t maxDig10    = std::numeric_limits<V>::max_digits10;
    constexpr size_t maxExp10    = std::numeric_limits<V>::max_exponent10;
    constexpr size_t maxExpDig10 = log10ceil(maxExp10);

    if(prec == size_t(-1)) // no Prec
    {
        if(is_oneof(fmt, fp_fmt{}, fp_fmt::scientific, fp_fmt::general))
        {
            // float : - 1 . 23456735 e- 36
            // double: - 1 . 2345678901234567 e- 100
            // sign+dot|all digs| e- |       exp
            return 2 + maxDig10 + 2 + std::max<size_t>(2, maxExpDig10);
        }
        else if(fmt == fp_fmt::hex)
        {
            // float : - 1 . fffffe p+ 127
            // double: - 1 . fffffffffffff p+ 1023
            // sign+dot|  all digs |   p+  |    exp
            return 2 + (dig2s + 3)/4 + 2 + maxExpDig10;
        }
        else if(fmt == fp_fmt::fixed)
        {
            return 2 + maxDig10 + maxExp10;
        }
    }
    else // has Prec
    {
        DSK_ASSERT(fmt != fp_fmt());

        if(is_oneof(fmt, fp_fmt::scientific, fp_fmt::general))
        {
            // -1. 26000 e+ 62. general fmt may be smaller as it trims trailing 0s.
            return 3 + prec + 2 + std::max<size_t>(2, maxExpDig10);
        }
        else if(fmt == fp_fmt::hex)
        {
            // -1. 260e0 p+ 6
            return 3 + prec + 2 + maxExpDig10;
        }
        else if(fmt == fp_fmt::fixed)
        {
            return 2 + 1 + maxExp10 + prec;
        }
    }

    DSK_ASSERT(false); // unknown fmt.
    return 0; // trigger error faster.
}


template<fp_fmt Fmt = {}, size_t Prec = size_t(-1)>
constexpr auto dsk_stringify(stringify_cpo, std::floating_point auto v, fp_fmt_constant<Fmt> = {},
                                                                        prec_constant<Prec> = {}) noexcept
{
    fixed_storage_str<stringify_fp_max_bytes<decltype(v)>(Fmt, Prec)> b;

    auto[e, ec] = [&]()
    {
             if constexpr(Fmt == fp_fmt{} && Prec == size_t(-1)) return std::to_chars(b.buf_begin(), b.buf_end(), v );
        else if constexpr(Fmt != fp_fmt{} && Prec == size_t(-1)) return std::to_chars(b.buf_begin(), b.buf_end(), v , Fmt);
        else if constexpr(Fmt != fp_fmt{} && Prec != size_t(-1)) return std::to_chars(b.buf_begin(), b.buf_end(), v , Fmt, Prec);
        else
            static_assert(false, "Fmt must be specified when Prec is specified.");
    }();

    DSK_ASSERT(ec == std::errc());

    b.set_end(e);
    return b;
}

template<fp_fmt Fmt = fp_fmt::general, size_t Prec>
constexpr auto dsk_stringify(stringify_cpo, std::floating_point auto v, prec_constant<Prec> prec,
                                                                        fp_fmt_constant<Fmt> fmt = {}) noexcept
{
    return dsk_stringify(stringify_cpo(), v, fmt, prec);
}

constexpr auto dsk_stringify(stringify_cpo, std::floating_point auto v, fp_fmt fmt) noexcept
{
    fixed_storage_str<stringify_fp_max_bytes<decltype(v)>(fp_fmt::fixed)> b; // use the largest buffer without prec

    auto[e, ec] = std::to_chars(b.buf_begin(), b.buf_end(), v, fmt);

    DSK_ASSERT(ec == std::errc());

    b.set_end(e);
    return b;
}

constexpr auto dsk_stringify(stringify_cpo, std::floating_point auto v, fp_fmt fmt, prec_t prec)
{
    string b;
    resize_buf(b, stringify_fp_max_bytes<decltype(v)>(fmt, prec.value));

    auto[e, ec] = std::to_chars(buf_begin(b), buf_end(b), v, fmt, static_cast<int>(prec.value));

    DSK_ASSERT(ec == std::errc());

    resize_buf(b, e - buf_begin(b));
    return b;
}

constexpr auto dsk_stringify(stringify_cpo, std::floating_point auto v, prec_t prec, fp_fmt fmt = fp_fmt::general)
{
    return dsk_stringify(stringify_cpo(), v, fmt, prec);
}


constexpr auto&&      dsk_stringify(stringify_cpo, _str_ auto&&  v) noexcept { return DSK_FORWARD(v); }
constexpr auto        dsk_stringify(stringify_cpo, _char_t_ auto v) noexcept { return std::array{v}; }
constexpr char const* dsk_stringify(stringify_cpo, _bool_ auto   v) noexcept { return v ? "true" : "false"; }
constexpr char const* dsk_stringify(stringify_cpo, std::nullptr_t ) noexcept { return "null"; }


} // namespace dsk
