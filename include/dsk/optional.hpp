#pragma once

#include <dsk/err.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/concepts.hpp>
#include <optional>
#include <functional>


namespace dsk{


using std::nullopt;
using std::nullopt_t;
using std::bad_optional_access;


template<class T>
class optional : public std::optional<T>
{
    using base = std::optional<T>;

public:
    using base::base;

#if 1//def _MSC_VER
    /// workaround for unconstrained operator CMP(std::optional, U const&)
    //  NOTE: CMP(optional, U const&) will convert U to optional<U>.
    //  TODO: remove when constrained

    friend constexpr auto operator<=>(optional const& l, optional const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) <=> static_cast<base const&>(r); })
    {
        return static_cast<base const&>(l) <=> static_cast<base const&>(r);
    }

    template<class U>
    friend constexpr auto operator<=>(optional const& l, std::optional<U> const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) <=> r; })
    {
        return static_cast<base const&>(l) <=> r;
    }

    template<class U>
    friend constexpr auto operator<=>(std::optional<U> const& l, optional const& r) noexcept
    requires(requires{
        l <=> static_cast<base const&>(r); })
    {
        return l <=> static_cast<base const&>(r);
    }


    friend constexpr bool operator==(optional const& l, optional const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) == static_cast<base const&>(r); })
    {
        return static_cast<base const&>(l) == static_cast<base const&>(r);
    }

    template<class U>
    friend constexpr bool operator==(optional const& l, std::optional<U> const& r) noexcept
    requires(requires{
        static_cast<base const&>(l) == r; })
    {
        return static_cast<base const&>(l) == r;
    }

    template<class U>
    friend constexpr bool operator==(std::optional<U> const& l, optional const& r) noexcept
    requires(requires{
        l == static_cast<base const&>(r); })
    {
        return l == static_cast<base const&>(r);
    }


    friend constexpr auto operator<=>(optional const& l, nullopt_t r) noexcept
    requires(requires{
        static_cast<base const&>(l) <=> r; })
    {
        return static_cast<base const&>(l) <=> r;
    }

    friend constexpr auto operator<=>(nullopt_t l,  optional const&r) noexcept
    requires(requires{
        l <=> static_cast<base const&>(r); })
    {
        return l <=> static_cast<base const&>(r);
    }

    friend constexpr bool operator==(optional const& l, nullopt_t r) noexcept
    requires(requires{
        static_cast<base const&>(l) == r; })
    {
        return static_cast<base const&>(l) == r;
    }
    
    friend constexpr bool operator==(nullopt_t l,  optional const&r) noexcept
    requires(requires{
        l == static_cast<base const&>(r); })
    {
        return l == static_cast<base const&>(r);
    }

#endif
};

template<class T>
optional(T) -> optional<T>;


template<class T>
class optional<T&> : public std::optional<std::reference_wrapper<T>>
{
    using base = std::optional<std::reference_wrapper<T>>;

    constexpr optional(base const& b) noexcept : base(b) {}

public:
    using value_type = T&;
    
    using base::base;

    // always return T&
    constexpr T& value() const { return base::value().get(); }
    constexpr T& operator* () const noexcept { return base::operator*().get(); }
    constexpr T* operator->() const noexcept { return std::addressof(**this); }

    constexpr T& value_or(T& defaultVal) const { return base::has_value() ? **this : defaultVal; }

    constexpr optional and_then(auto&& f) const
    {
        return base::and_then([&](T& t) ->decltype(auto) { return std::invoke(DSK_FORWARD(f), t); });
    }

    constexpr optional transform(auto&& f) const
    {
        return base::transform([&](T& t) ->decltype(auto) { return std::invoke(DSK_FORWARD(f), t); });
    }

    constexpr optional or_else(auto&& f) const
    {
        return base::or_else([&](T& t) ->decltype(auto) { return std::invoke(DSK_FORWARD(f), t); });
    }

    friend constexpr void swap(optional& l, optional& r) noexcept
    {
        l.swap(r);
    }
};


template<_void_ T>
class optional<T>
{
    bool _has = false;

public:
    using value_type = void;

    constexpr optional() = default;

    constexpr explicit optional(in_place_t) noexcept : _has(true) {}

    constexpr optional(nullopt_t) noexcept : _has(false) {}
    constexpr optional& operator=(nullopt_t) noexcept { _has = false; return *this; }

    constexpr explicit operator bool() const noexcept { return _has; }
    constexpr bool has_value() const noexcept { return _has; }
    constexpr void     value() const          { if(! _has) throw bad_optional_access(); }
    constexpr void operator*() const noexcept { DSK_ASSERT(_has); }

    constexpr void swap(optional& r) noexcept { std::swap(_has, r._has); }
    constexpr void reset() noexcept { _has = false; }
    constexpr void emplace() noexcept { _has = true; }

    constexpr auto operator<=>(optional const&) const noexcept = default;
    constexpr auto operator<=>(nullopt_t) const noexcept { return _has <=> false; }
    constexpr bool operator== (nullopt_t) const noexcept { return _has ==  false; }

    friend constexpr void swap(optional& l, optional& r) noexcept
    {
        l.swap(r);
    }
};


struct optional_impl_cpo{};
template<class T> void dsk_optional_impl(optional_impl_cpo, optional<T>&);
template<class T> void dsk_optional_impl(optional_impl_cpo, std::optional<T>&);
template<class T> concept _optional_ = requires(std::remove_cvref_t<T>& t){ dsk_optional_impl(optional_impl_cpo(), t); };

template<_optional_ T> using optional_value_t = typename std::remove_cvref_t<T>::value_type;

template<class T, class V> concept _optional_of_ = _same_as_<optional_value_t<T>, V> && _optional_<T>;


constexpr decltype(auto) assure_val(_optional_ auto& t, auto&&... args)
{
    if(t) return *t;
    else  return  t.emplace(DSK_FORWARD(args)...);
}


inline constexpr struct has_val_cpo
{
    constexpr bool operator()(auto const& t) const noexcept
    requires(requires{
        dsk_has_val(*this, t); })
    {
        return dsk_has_val(*this, t);
    }

    constexpr bool operator()(auto const& t) const noexcept
    requires(requires{
        has_err(t); })
    {
        return ! has_err(t);
    }
} has_val;

inline constexpr struct get_val_cpo
{
    constexpr decltype(auto) operator()(auto&& t) const noexcept
    requires(requires{
        dsk_get_val(*this, DSK_FORWARD(t)); })
    {
        DSK_ASSERT(has_val(t));
        return dsk_get_val(*this, DSK_FORWARD(t));
    }
} get_val;


template<class T>
concept _valuable_ = requires(T t)
{
    has_val(t);
    get_val(t);
};

struct val_t_cpo{};

template<_valuable_ T>
using val_t = decltype(dsk_val_t(val_t_cpo(), std::declval<T&>()));

template<class T> auto dsk_val_t(val_t_cpo, T const&) -> typename T::value_type;


constexpr bool dsk_has_val(has_val_cpo, _optional_ auto const& t) noexcept
{
    return t.has_value();
}

constexpr decltype(auto) dsk_get_val(get_val_cpo, _optional_ auto&& t) noexcept
{
    return *DSK_FORWARD(t);
}


template<void_result_e VE = void_as_void, _valuable_ T>
constexpr decltype(auto) take_value(T&& t)
{
    if constexpr(_void_<val_t<T>>) return return_void<VE>();
    else                           return val_t<T>(get_val(std::move(t)));
}


} // namespace dsk
