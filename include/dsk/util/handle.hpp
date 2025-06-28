#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/concepts.hpp>
#include <compare>


namespace dsk{


// like std::unique_ptr but allow any handle like type,
// and also copyable if provide a copier,
// Del/Cpy can also make handle ref counted, i.e., share semantic.
template<
    class H,
    auto  Del,
    auto  Cpy,
    auto  Null = H{}
>
class handle_t
{
    static constexpr bool h_is_ptr = std::is_pointer_v<H>;
    static constexpr bool copyable = ! _null_op_<decltype(Cpy)>;

    H _h{Null};

    H copy_handle() const noexcept requires(copyable)
    {
        return valid() ? Cpy(_h) : Null;
    }

public:
    using value_type = H;

    constexpr handle_t() = default;

    constexpr explicit handle_t(H h) noexcept : _h{h} {}

    ~handle_t() { reset(); }

    constexpr handle_t(handle_t&& r) noexcept : _h{r.release()} {}

    constexpr handle_t& operator=(handle_t&& r) noexcept
    {
        if(this != std::addressof(r))
        {
            reset(r.release());
        }
        return *this;
    }

    constexpr handle_t(handle_t const& r) noexcept requires(copyable)
        : _h{r.copy_handle()}
    {}

    constexpr handle_t& operator=(handle_t const& r) noexcept requires(copyable)
    {
        if(this != std::addressof(r))
        {
            reset(r.copy_handle());
        }
        return *this;
    }

    constexpr bool valid() const noexcept { return _h != Null; }
    constexpr explicit operator bool() const noexcept { return valid(); }

    constexpr H get() const noexcept { return _h; }
    constexpr H checked_get() const noexcept { DSK_ASSERT(valid()); return _h; }

    constexpr H handle() const noexcept { return _h; }
    constexpr H checked_handle() const noexcept { DSK_ASSERT(valid()); return _h; }

    constexpr void close() noexcept
    {
        reset();
    }

    constexpr void reset(H h = Null) noexcept
    {
        if(valid())
            Del(_h);
        _h = h;
    }

    constexpr void reset_handle(H h = Null) noexcept
    {
        reset(h);
    }

    [[nodiscard]] constexpr H release() noexcept
    {
        return std::exchange(_h, Null);
    }

    constexpr void swap(handle_t& r) noexcept
    {
        std::swap(_h, r._h);
    }

    constexpr auto* operator->() noexcept
    {
        DSK_ASSERT(valid());
        if constexpr(h_is_ptr) return  _h;
        else                   return &_h;
    }

    constexpr auto* operator->() const noexcept
    {
        DSK_ASSERT(valid());
        if constexpr(h_is_ptr) return  _h;
        else                   return &_h;
    }

    constexpr auto& operator*() noexcept
    {
        DSK_ASSERT(valid());
        if constexpr(h_is_ptr) return *_h;
        else                   return  _h;
    }

    constexpr auto& operator*() const noexcept
    {
        DSK_ASSERT(valid());
        if constexpr(h_is_ptr) return *_h;
        else                   return  _h;
    }

    constexpr auto operator<=>(handle_t const& r) const noexcept = default;
};

template<class H, auto D, auto C, auto N> void _handle_t_impl(handle_t<H, D, C, N>&);
template<class T> concept _handle_t_ = requires(std::remove_cvref_t<T>& t){ dsk::_handle_t_impl(t); };

constexpr auto dsk_get_handle(get_handle_cpo, _handle_t_ auto& h) noexcept
{
    return h.get();
}


template<class H, auto Del, auto Null = H{}>
using unique_handle = handle_t<H, Del, null_op, Null>;

template<class H, auto Del, auto Cpy, auto Null = H{}>
using copyable_handle = handle_t<H, Del, Cpy, Null>;

template<class H, auto Unref, auto Ref, auto Null = H{}>
using shared_handle = handle_t<H, Unref, Ref, Null>;


} // namespace dsk


namespace std{

template<class H, auto D, auto C, auto N>
struct hash<dsk::handle_t<H, D, C, N>>
{
    auto operator()(dsk::handle_t<H, D, C, N> const& h) const noexcept
    {
        return hash<H>()(h.get());
    }
};

} // namespace std
