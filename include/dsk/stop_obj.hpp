#pragma once

#include <dsk/util/concepts.hpp>
#include <dsk/util/function.hpp>
#include <optional>
#include <stop_token>


namespace dsk{


template<class T> concept _stop_source_ = _no_cvref_same_as_<T, std::stop_source>;
template<class T> concept _stop_token_  = _no_cvref_same_as_<T, std::stop_token>;

template<class T>
concept _stop_source_or_ref_wrap_ = _stop_source_<T> || _ref_wrap_of_<T, std::stop_source>;


[[nodiscard]] auto   get_stop_token(std::stop_source const& s) noexcept { return s.get_token(); }
[[nodiscard]] auto&& get_stop_token(_stop_token_ auto&&     t) noexcept { return DSK_FORWARD(t); }

template<class T> concept _stop_obj_ = requires(T t){ dsk::get_stop_token(t); };


// once set, cannot be moved/copied.
template<class T>
class once_optional
{
    union{ char _dummy = 0; T _val; };
    bool _engaged = false;

public:
    constexpr once_optional() {}
    constexpr ~once_optional() { reset(); }

    constexpr once_optional([[maybe_unused]] once_optional const& r) noexcept { DSK_ASSERT(! r._engaged); }
    constexpr once_optional([[maybe_unused]] once_optional&& r) noexcept { DSK_ASSERT(! r._engaged); }
    constexpr once_optional& operator=([[maybe_unused]] once_optional const& r) noexcept { DSK_ASSERT(! _engaged && ! r._engaged); return *this; }
    constexpr once_optional& operator=([[maybe_unused]] once_optional&& r) noexcept { DSK_ASSERT(! _engaged && ! r._engaged); return *this; }

    constexpr explicit operator bool() const noexcept { return _engaged; }
    constexpr bool has_value() const noexcept { return _engaged; }

    constexpr auto* operator->(this auto&& s) noexcept { DSK_ASSERT(s._engaged); return std::addressof(s._val); }
    constexpr auto&& operator*(this auto&& s) noexcept { DSK_ASSERT(s._engaged); return DSK_FORWARD(s)._val; }

    constexpr auto&& value(this auto&& s)
    {
        if(s._engaged) return DSK_FORWARD(s)._val;
        else           throw bad_optional_access();
    }

    constexpr void reset()
    {
        if(_engaged)
        {
            if constexpr(! std::is_trivially_destructible_v<T>)
            {
                _val.~T();
            }

            _engaged = false;
        }
    }

    constexpr T& emplace(auto&&... args)
    {
        reset();

        std::construct_at(std::addressof(_val), DSK_FORWARD(args)...);	
        _engaged = true;

        return _val;
    }
};


template<class F = unique_function<void()>>
class optional_stop_callback_t
    : public once_optional<std::stop_callback<F>>
{
    using base = once_optional<std::stop_callback<F>>;

public:
    auto& emplace(_stop_obj_ auto&& s, auto&& f)
    {
        return base::emplace(get_stop_token(DSK_FORWARD(s)), DSK_FORWARD(f));
    }

    void emplace_if_stop_possible(std::stop_source const& s, auto&& f)
    {
        if(s.stop_possible())
        {
            base::emplace(s.get_token(), DSK_FORWARD(f));
        }
    }
};

template<class F>
optional_stop_callback_t(auto, F) -> optional_stop_callback_t<F>;


using optional_stop_callback = optional_stop_callback_t<>;


} // namespace dsk
