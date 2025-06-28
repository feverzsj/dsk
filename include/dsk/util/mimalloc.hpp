#pragma once

#include <dsk/util/allocator.hpp>
#include <mimalloc.h>


namespace dsk{


// if define the lambda directly in template, each time use it, a different identity will be initialized.
inline constexpr auto _mi_aligned_alloc = [](align_val_t align, size_t size) noexcept
{
    return mi_malloc_aligned(size, static_cast<size_t>(align));
};

// don't use type alias, which doesn't play well with std::allocator_traits<Al>::rebind_alloc
template<class T>
struct mi_allocator : min_aligned_allocator_base<T, _mi_aligned_alloc, mi_free>
{
    constexpr mi_allocator() = default;

    template<class U>
    constexpr mi_allocator(mi_allocator<U> const&) noexcept
    {}

    template<class U>
    constexpr bool operator==(mi_allocator<U> const&) const noexcept
    {
        return true;
    }
};


struct mi_c_alloc
{
#ifdef _MSC_VER // dllimport-ed function cannot be used as constexpr
    inline static const auto malloc  = mi_malloc;
    inline static const auto calloc  = mi_calloc;
    inline static const auto realloc = mi_realloc;
    inline static const auto free    = mi_free;
    inline static const auto aligned_alloc = mi_aligned_alloc;
#else
    static constexpr auto malloc  = mi_malloc;
    static constexpr auto calloc  = mi_calloc;
    static constexpr auto realloc = mi_realloc;
    static constexpr auto free    = mi_free;
    static constexpr auto aligned_alloc = mi_aligned_alloc;
#endif
};


} // namespace dsk
