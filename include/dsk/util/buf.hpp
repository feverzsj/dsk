#pragma once

#include <dsk/util/string.hpp>
#include <dsk/util/concepts.hpp>
#include <ranges>
#include <utility>
#if __has_include(<boost/container/container_fwd.hpp>)
#include <boost/container/container_fwd.hpp>
#endif


namespace dsk{

                                          //v: avoid decaying c-array
template<class B> concept _buf_ = requires(B& b){ std::ranges::data(b); std::ranges::size(b); };

template<_buf_ B> using buf_val_t = std::iter_value_t<decltype(std::ranges::data(std::declval<B&>()))>;

template<class B> concept _buf_class_ = _class_<B> && _buf_<B>;

// NOTE: the content not the buf itself
template<class B> concept _trivially_copyable_buf_ = std::is_trivially_copyable_v<buf_val_t<B>> && _buf_<B>;

template<class B, class E> concept _buf_of_ = _same_as_<buf_val_t<B>, E> && _buf_<B>;
template<class T, class E> concept _buf_of_or_same_as_ = _buf_of_<T, E> || _same_as_<T, E>;

template<class B> concept   _char_buf_ = _buf_of_<B, char>;
template<class B> concept _char_t_buf_ = _char_t_<buf_val_t<B>> && _buf_<B>;
template<class B> concept   _byte_buf_ =   _byte_<buf_val_t<B>> && _buf_<B>;

template<class B> concept _borrowed_buf_      = std::ranges::borrowed_range<B> && _buf_<B>;
template<class B> concept _borrowed_byte_buf_ = _byte_<buf_val_t<B>> && _borrowed_buf_<B>;

template<class B, class B1> concept _similar_buf_        = _same_as_<buf_val_t<B>, buf_val_t<B1>>;
template<class T, class B1> concept _similar_buf_or_val_ = _similar_buf_<T, B1> || _same_as_<T, buf_val_t<B1>>;

template<class B>
concept _resizable_buf_ = requires(std::remove_cvref_t<B> b){ b.resize(1); } && _buf_<B>;

template<class B, class E>
concept _resizable_buf_of_ = _resizable_buf_<B> && _same_as_<E, buf_val_t<B>>;

template<class B> concept   _resizable_char_buf_ = _resizable_buf_of_<B, char>;
template<class B> concept   _resizable_byte_buf_ = _resizable_buf_<B> && _byte_<buf_val_t<B>>;
template<class B> concept _resizable_16bits_buf_ = _resizable_buf_<B> && _16bits_<buf_val_t<B>>;

template<class B>
concept _reservable_buf_ = requires(std::remove_cvref_t<B> b){ b.reserve(1); } && _resizable_buf_<B>;

template<class B, class E>
concept _reservable_buf_of_ = _reservable_buf_<B> && _same_as_<E, buf_val_t<B>>;


constexpr auto* buf_data(_buf_ auto&& b) noexcept
{
    // NOTE: not using DSK_FORWARD(b) here to allow
    //       binding to rval ref to non borrowed range.
    //       So, be careful with dangling reference.
    return std::ranges::data(b);
}

template<class T>
constexpr auto* buf_data(_buf_ auto&& b) noexcept
requires(
    sizeof(T) == sizeof(buf_val_t<decltype(b)>))
{
    return alias_cast<T>(buf_data(DSK_FORWARD(b)));
}

template<_void_ T>
constexpr auto* buf_data(_buf_ auto&& b) noexcept
{
    return buf_data(DSK_FORWARD(b));
}


constexpr bool buf_empty(_buf_ auto const& b) noexcept
{
    return std::ranges::empty(b);
}


template<class T = size_t>
constexpr T buf_size(_buf_ auto const& b) noexcept
{
    return static_cast<T>(std::ranges::size(b));
}

template<class T = size_t>
constexpr T buf_byte_size(_buf_ auto const& b) noexcept
{
    return static_cast<T>(buf_size(b) * sizeof(buf_val_t<decltype(b)>));
};


constexpr auto&& buf_front(_buf_ auto&& b) noexcept
{
    DSK_ASSERT(buf_size(b));
    return std::forward_like<decltype(b)>(*buf_data(b));
};

constexpr auto&& buf_back(_buf_ auto&& b) noexcept
{
    DSK_ASSERT(buf_size(b));
    return std::forward_like<decltype(b)>(*(buf_data(b) + buf_size(b) - 1));
};

constexpr auto* buf_begin(_buf_ auto&& b) noexcept { return buf_data(DSK_FORWARD(b)); }
constexpr auto*   buf_end(_buf_ auto&& b) noexcept { return buf_data(DSK_FORWARD(b)) + buf_size(b); }


template<size_t N, class T>
constexpr auto make_span(T* d) noexcept
{
    return std::span<T, N>(d, N);
}


template<class T, class S>
constexpr decltype(auto) as_buf_of(S&& s) noexcept
{
    if constexpr(std::convertible_to<S&, T&>)
    {
        static_assert(_lref_<S>);

        return make_span<1>(similar_cast<T>(std::addressof(s)));
    }
    else if constexpr(_buf_<S>)
    {
        static_assert(std::ranges::borrowed_range<S>);

        using vt = buf_val_t<S>;

        if constexpr(_same_as_<vt, T>)
        {
            return DSK_FORWARD(s);
        }
        else if constexpr(sizeof(vt) == sizeof(T) && std::convertible_to<vt&, T&>)
        {
            return std::span(similar_cast<T>(buf_data(s)), buf_size(s));
        }
        else if constexpr(_trivially_copyable_<vt> && _trivially_copyable_<T>)
        {
            if constexpr(sizeof(vt) % sizeof(T) == 0)
            {
                return std::span(alias_cast<T>(buf_data(s)), buf_size(s) * (sizeof(vt) / sizeof(T)));
            }
            else
            {
                size_t nByte = buf_byte_size(s);
    
                DSK_ASSERT(nByte % sizeof(T) == 0);

                return std::span(alias_cast<T>(buf_data(s)), nByte / sizeof(T));
            }
        }
        else
        {
            static_assert(false, "unsupported type");
        }
    }
    else if constexpr(_trivially_copyable_<S>)
    {
        static_assert(_lref_<S>);
        static_assert(sizeof(s) % sizeof(T) == 0);

        return make_span<sizeof(s) / sizeof(T)>(alias_cast<T>(std::addressof(s)));
    }
    else
    {
        static_assert(false, "unsupported type");
    }
}


template<_buf_ B>
constexpr decltype(auto) as_similar_buf(auto&& u) noexcept
{
    return as_buf_of<buf_val_t<B>>(DSK_FORWARD(u));
}


template<class T = size_t>
constexpr T buf_capacity(_reservable_buf_ auto const& b) noexcept
{
    return static_cast<T>(b.capacity());
}

constexpr void reserve_buf(_reservable_buf_ auto& b, size_t n)
{
    b.reserve(n);
}


enum resize_policy_e : unsigned
{
    minimum_growth = 1,
    use_all_capacity = 1 << 1,
};


template<unsigned Policy = 0>
constexpr auto resize_buf(_resizable_buf_ auto& b, size_t n)
{
    if constexpr(_reservable_buf_<decltype(b)>)
    {
        if constexpr(test_flag(Policy, minimum_growth))
        {
            reserve_buf(b, n);

            if constexpr(test_flag(Policy, use_all_capacity))
            {
                n = buf_capacity(b);
            }
        }
        else if constexpr(test_flag(Policy, use_all_capacity))
        {
            n = std::max(n, buf_capacity(b));
        }
    }

    if constexpr(requires{ b.resize_and_overwrite(n, [n](auto&&...){ return n; }); }) // c++23 std::basic_string
    {
        b.resize_and_overwrite(n, [n](auto&&...){ return n; });
    }
    else if constexpr(requires{ b.__resize_default_init(n); }) // libc++ >= 8.0 std::basic_string
    {
        b.__resize_default_init(n);
    }
#if __has_include(<boost/container/container_fwd.hpp>)
    else if constexpr(requires{ b.resize(n, boost::container::default_init); })
    {
        b.resize(n, boost::container::default_init);
    }
#endif
    else
    {
        b.resize(n);
    }

    if constexpr(test_flag(Policy, use_all_capacity))
    {
        return n;
    }
}

template<unsigned Policy = 0>
constexpr size_t resize_buf_at_least(_resizable_buf_ auto& b, size_t n)
{
    return resize_buf<use_all_capacity | Policy>(b, n);
}


constexpr void remove_buf_tail(_resizable_buf_ auto& b, size_t n)
{
    resize_buf(b, buf_size(b) - n);
}


constexpr void clear_buf(_buf_ auto& b)
{
    if constexpr(requires{ b.clear(); }) b.clear();
    else                                 resize_buf(b, 0);
}

// resize without preserving existing data
template<unsigned Policy = 0>
constexpr void rescale_buf(_buf_ auto& b, size_t n)
{
    if constexpr(requires{ b.rescale(n); })
    {
        b.rescale(n);
    }
    else
    {
        clear_buf(b);
        resize_buf<Policy>(b, n);
    }
}


// "buy" a sub buffer of size 'n' from 'b' start at end of 'b',
// return the sub buffer as a std::span if Policy & use_all_capacity,
// otherwise return a pointer to the sub buffer
template<class T = void, unsigned Policy = 0>
constexpr auto buy_buf(_resizable_buf_ auto& b, size_t n)
{
    size_t off = buf_size(b);

    if constexpr(test_flag(Policy, use_all_capacity))
    {
        size_t rs  = resize_buf<Policy>(b, off + n);
        return std::span(buf_data<T>(b) + off, rs - off);
    }
    else
    {
        resize_buf<Policy>(b, off + n);
        return buf_data<T>(b) + off;
    }
}

template<class T = void, unsigned Policy = 0>
constexpr auto buy_buf_at_least(_resizable_buf_ auto& b, size_t n)
{
    return buy_buf<T, use_all_capacity | Policy>(b, n);
}



template<class T>
constexpr T* append_buf(T* d, _buf_of_<T> auto const& s)
{
    return std::copy_n(buf_data(s), buf_size(s), d);
}

template<class T>
constexpr T* append_buf(T* d, _buf_of_<T> auto const&... ss)
{
    (... , (d = append_buf(d, ss)));
    return d;
}

constexpr void append_buf(_resizable_buf_ auto& d, _similar_buf_<decltype(d)> auto const&... ss)
{
    if constexpr(sizeof...(ss))
    {
        append_buf(
            buy_buf(d, (... + buf_size(ss))),
            ss...);
    }
}


template<_buf_ D>
constexpr void assign_buf(D& d, buf_val_t<D> const* s, size_t n)
{
    if constexpr(requires{ d.assign(s, n); }) // std::string
    {
        d.assign(s, n);
    }
    else if constexpr(requires{ d.assign(s, s + n); }) // std::vector
    {
        d.assign(s, s + n);
    }
    else if constexpr(_resizable_buf_<D>)
    {
        rescale_buf(d, n);
        std::copy_n(s, n, buf_data(d));
    }
    else if constexpr(requires{ d = D(s, n); }) // std::span, std::string_view
    {
        d = D(s, n);
    }
    else
    {
        static_assert(false, "unsupported assignment");
    }
}

constexpr void assign_buf(_buf_ auto& d, _similar_buf_<decltype(d)> auto&& s)
{
    if constexpr(requires{ d = DSK_FORWARD(s); })
    {
        d = DSK_FORWARD(s);
    }
    else
    {
        assign_buf(d, buf_data(s), buf_size(s));
    }
}


template<_resizable_buf_ D = string>
constexpr D cat_buf(_similar_buf_<D> auto&&... ss)
{
    D d;

    if constexpr(sizeof...(ss) == 1) assign_buf(d, DSK_FORWARD(ss)...);
    else                             append_buf(d, ss...);

    return d;
}



template<_buf_ B, class T>
struct cast_buf_wrapper
{
    using value_type = T;
    using ori_value_type = buf_val_t<B>;

    static_assert(sizeof(ori_value_type) % sizeof(T) == 0
                  || sizeof(T) % sizeof(ori_value_type) == 0);

    static constexpr bool mutable_buf = ! std::is_const_v<B>;

    B& b;

    constexpr explicit cast_buf_wrapper(B& b_) noexcept : b(b_) {}

    constexpr size_t size() const noexcept { return ori_size_to_t(buf_size(b)); }

    constexpr T const* data() const noexcept { return static_cast<T const*>(buf_data(b)); }
    constexpr T const* begin() const noexcept { return data(); }
    constexpr T const* end  () const noexcept { return data() + size(); }

    constexpr T* data () noexcept requires(mutable_buf) { return static_cast<T*>(buf_data(b)); }
    constexpr T* begin() noexcept requires(mutable_buf) { return data(); }
    constexpr T* end  () noexcept requires(mutable_buf) { return data() + size(); }

    constexpr void rescale(size_t n) requires(mutable_buf && _resizable_buf_<B>)
    {
        rescale_buf(b, t_size_to_ori(n));
    }

    constexpr void resize(size_t n) requires(mutable_buf && _resizable_buf_<B>)
    {
        resize_buf(b, t_size_to_ori(n));
    }

    constexpr void clear() requires(mutable_buf && _resizable_buf_<B>)
    {
        clear_buf(b);
    }


    static constexpr size_t ori_size_to_t(size_t n) noexcept
    {
        if constexpr(sizeof(ori_value_type) < sizeof(T))
        {
            static_assert(sizeof(T) % sizeof(ori_value_type) == 0);
            DSK_ASSERT(n * sizeof(ori_value_type) % sizeof(T) == 0);
            return (n * sizeof(ori_value_type)) / sizeof(T);
        }
        else
        {
            static_assert(sizeof(ori_value_type) % sizeof(T) == 0);
            constexpr size_t m = sizeof(ori_value_type) / sizeof(T);
            return n * m;
        }
    }

    static constexpr size_t t_size_to_ori(size_t n) noexcept
    {
        if constexpr(sizeof(T) < sizeof(ori_value_type))
        {
            static_assert(sizeof(ori_value_type) % sizeof(T) == 0);
            DSK_ASSERT(n * sizeof(T) % sizeof(ori_value_type) == 0);
            return (n * sizeof(T)) / sizeof(ori_value_type);
        }
        else
        {
            static_assert(sizeof(T) % sizeof(ori_value_type) == 0);
            constexpr size_t m = sizeof(T) / sizeof(ori_value_type);
            return n * m;
        }
    }
};


// reinterpret cast buf to be _buf_of_<NewBuf, T>
template<class T>
constexpr auto cast_buf(_buf_ auto& b) noexcept
{
    if constexpr(_buf_of_<decltype(b), T>) return b;
    else                                   return cast_buf_wrapper<DSK_NO_REF_T(b), T>(b);
}


} // namespace dsk
