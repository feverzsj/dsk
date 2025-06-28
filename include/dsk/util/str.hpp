#pragma once

#include <dsk/config.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/string.hpp>
#include <span>
#include <string_view>


namespace dsk{


constexpr auto* str_data(_char_t_ auto* s) noexcept { return s; }
constexpr auto* str_data(_char_t_buf_ auto&& s) noexcept { return buf_data(DSK_FORWARD(s)); }

template<class T = size_t, _char_t_ E>
constexpr T str_size(E const* s) noexcept { return static_cast<T>(std::char_traits<E>::length(s)); }

template<class T = size_t>
constexpr T str_size(_char_t_buf_ auto const& s) noexcept { return buf_size<T>(s); }

constexpr bool str_empty(_char_t_ auto* s) noexcept { return !s || !*s; }
constexpr bool str_empty(_char_t_buf_ auto const& s) noexcept { return buf_empty(s); }

// return a _buf_
constexpr auto   str_range(_char_t_ auto* s) noexcept { return std::span(s, str_size(s)); }
constexpr auto&& str_range(_char_t_buf_ auto&& s) noexcept { return DSK_FORWARD(s); }


template<class T> concept _str_ = requires(std::remove_reference_t<T> t){ dsk::str_data(t); };
template<class T> concept _str_class_ = _class_<T> && _str_<T>;

template<class T> using str_val_t = std::iter_value_t<decltype(str_data(std::declval<T&>()))>;

template<class T, class E> concept _str_of_   = _same_as_<str_val_t<T>, E>;
template<class T         > concept _char_str_ = _str_of_<T, char>;
template<class T         > concept _byte_str_ = _byte_<str_val_t<T>> && _str_<T>;

template<class T, class E> concept _similar_str_of_ = sizeof(str_val_t<T>) == sizeof(E) && _char_t_<E>;
template<class T, class U> concept _similar_str_    = _similar_str_of_<T, str_val_t<U>>;

template<class T         > concept _unwrap_str_    = _str_<unwrap_ref_t<T>>;
template<class T, class E> concept _unwrap_str_of_ = _str_of_<unwrap_ref_t<T>, E>;

template<class T> concept _borrowed_str_      = std::ranges::borrowed_range<T> && _str_<T>;
template<class T> concept _borrowed_byte_str_ = _byte_<str_val_t<T>> && _borrowed_str_<T>;


template<class T = size_t>
constexpr T str_byte_size(_str_ auto const& b) noexcept
{
    return static_cast<T>(str_size(b) * sizeof(str_val_t<decltype(b)>));
};


constexpr auto* str_begin(_str_ auto&& s) noexcept { return str_data(DSK_FORWARD(s)); }
constexpr auto*   str_end(_str_ auto&& s) noexcept { return str_data(DSK_FORWARD(s)) + str_size(s); }


constexpr decltype(auto) str_view(_str_ auto&& s) noexcept
{
    if constexpr(_specialized_from_<decltype(s), std::basic_string_view>)
    {
        return DSK_FORWARD(s);
    }
    else if constexpr(requires{ std::basic_string_view(DSK_FORWARD(s)); })
    {
        return std::basic_string_view(DSK_FORWARD(s));
    }
    else
    {
        return std::basic_string_view<str_val_t<decltype(s)>>(str_data(s), str_size(s));
    }
}


template<class T>
auto* str_data(_str_ auto&& s) noexcept
requires(
    sizeof(T) == sizeof(str_val_t<decltype(s)>))
{
    return alias_cast<T>(str_data(DSK_FORWARD(s)));
}

template<class T>
constexpr decltype(auto) as_str_of(_str_ auto&& s) noexcept
{
    if constexpr(_same_as_<T, str_val_t<decltype(s)>>)
    {
        return DSK_FORWARD(s);
    }
    else
    {
        return std::span(str_data<T>(DSK_FORWARD(s)), str_size(s));
    }
}

template<class T>
constexpr decltype(auto) str_view(_str_ auto const& s) noexcept
{
    if constexpr(_same_as_<T, str_val_t<decltype(s)>>)
    {
        return str_view(s);
    }
    else
    {
        return std::basic_string_view(str_data<T>(s), str_size(s));
    }
}

template<class T>
constexpr auto* str_begin(_str_ auto&& s) noexcept { return str_data<T>(DSK_FORWARD(s)); }
template<class T>
constexpr auto* str_end(_str_ auto&& s) noexcept { return str_data<T>(DSK_FORWARD(s)) + str_size(s); }

template<class T>
constexpr auto str_range(_char_t_ auto* s) noexcept { return str_range(str_data<T>(s)); }
template<class T>
constexpr decltype(auto) str_range(_char_t_buf_ auto&& s) noexcept { return as_str_of<T>(DSK_FORWARD(s)); }


struct cstr_data_cpo{};

constexpr auto* cstr_data(auto&& t) noexcept
requires(
    requires{ dsk_cstr_data(cstr_data_cpo(), DSK_FORWARD(t)); })
{
    return dsk_cstr_data(cstr_data_cpo(), DSK_FORWARD(t));
}

constexpr auto* cstr_data(_char_t_ auto* s) noexcept
{
    return s;
}

constexpr auto* cstr_data(auto&& t) noexcept
requires(
    requires{ DSK_FORWARD(t).c_str(); })
{
    return DSK_FORWARD(t).c_str();
}


template<class T> concept _cstr_ = requires(T t){ cstr_data(t); } && _str_<T>;
template<class T> concept _byte_cstr_ = _byte_<str_val_t<T>> && _cstr_<T>;

static_assert(_cstr_<char const*>);
static_assert(_cstr_<char const[]>);
static_assert(_cstr_<char const[6]>);
static_assert(_cstr_<char const(&)[6]>);
static_assert(_cstr_<char []>);
static_assert(_cstr_<char (&)[6]>);


template<class T>
auto* cstr_data(_cstr_ auto&& s) noexcept
requires(
    sizeof(T) == sizeof(str_val_t<decltype(s)>))
{
    return alias_cast<T>(cstr_data(DSK_FORWARD(s)));
}


// use cstr_data(as_cstr(s)) to get cstr data
// which may point to a temporary that exists until the end of whole expression.
constexpr decltype(auto) as_cstr(_str_ auto&& s)
{
    if constexpr(_cstr_<decltype(s)>)
    {
        return DSK_FORWARD(s);
    }
    else if constexpr(requires{ basic_string(DSK_FORWARD(s)); })
    {
        return basic_string(DSK_FORWARD(s));
    }
    else
    {
        return basic_string<str_val_t<decltype(s)>>(str_data(s), str_size(s));
    }
}


template<class T>
constexpr T* append_str(T* d, _similar_str_of_<T> auto const&... ss)
{
    return append_buf(d, str_range<T>(ss)...);
}

constexpr void append_str(_resizable_buf_ auto& d, _similar_str_<decltype(d)> auto const&... ss)
{
    append_buf(d, str_range<buf_val_t<decltype(d)>>(ss)...);
}


constexpr void assign_str(_buf_ auto& d, _similar_str_<decltype(d)> auto&& s)
{
    if constexpr(requires{ d = DSK_FORWARD(s); })
    {
        d = DSK_FORWARD(s);
    }
    else
    {
        assign_buf(d, str_data<buf_val_t<decltype(d)>>(s), str_size(s));
    }
}


template<_resizable_buf_ D = string>
constexpr D cat_str(_similar_str_<D> auto&&... ss)
{
    D d;

    if constexpr(sizeof...(ss) == 1) assign_str(d, DSK_FORWARD(ss)...);
    else                             append_str(d, ss...);
    
    return d;
}


template<_resizable_buf_ D = string>
decltype(auto) cat_or_fwd_str(_similar_str_<D> auto&&... ss)
{
         if constexpr(sizeof...(ss) == 0) return empty_range<char>;
    else if constexpr(sizeof...(ss) == 1) return fwd_as_tuple(DSK_FORWARD(ss)...).first();
    else                                  return cat_str<D>(DSK_FORWARD(ss)...);
}


} // namespace dsk
