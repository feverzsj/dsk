#pragma once

#include <dsk/config.hpp>
#include <dsk/util/concepts.hpp>


namespace dsk{


// a copyable ref holder, can hold either lvalue or rvalue ref
template<_ref_ T>
struct ref_holder
{
    using ref_type = T;

    std::remove_reference_t<T>* p = nullptr;

    // ref_holder(_similar_ref_to_<T> auto&& v), could match e.g.: int if v is int&&, which may be surprising.
    constexpr ref_holder(auto&& v) noexcept requires(_similar_ref_to_<decltype(v), T>)
        : p(std::addressof(v))
    {}

    template<_similar_ref_to_<T> U>
    constexpr ref_holder(ref_holder<U> const& u) noexcept
        : p(u.p)
    {}

    template<_similar_ref_to_<T> U>
    constexpr ref_holder& operator=(ref_holder<U> const& u) noexcept
    {
        p = u.p;
    }

    //constexpr operator T() const noexcept { return *p; }
    constexpr T get() const noexcept { return static_cast<T>(*p); }
    constexpr auto* address() const noexcept { return p; }

    constexpr auto operator<=>(ref_holder const& r) const noexcept
    requires(
        requires{ *p <=> *r.p; })
    {
        return *p <=> *r.p;
    }

    constexpr bool operator==(ref_holder const& r) const noexcept
    requires(
        requires{ *p == *r.p; })
    {
        return *p == *r.p;
    }
};

template<class T>
ref_holder(T&&) -> ref_holder<T&&>;

template<_lref_ T>
using lref_holder = ref_holder<T>;


template<class T> void _ref_holder_impl(ref_holder<T>&);
template<class T> concept _ref_holder_ = requires(std::remove_cvref_t<T>& t){ dsk::_ref_holder_impl(t); };


constexpr auto* dsk_get_handle(get_handle_cpo, _ref_holder_ auto& r) noexcept
{
    return r.address();
}


} // namespace dsk


namespace std{

template<class T>
struct hash<dsk::ref_holder<T>>
{
    auto operator()(dsk::ref_holder<T> const& r) noexcept
    {
        return hash<T*>()(r.address());
    }
};

} // namespace std