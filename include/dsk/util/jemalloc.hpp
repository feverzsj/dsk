#pragma once

#include <dsk/util/allocator.hpp>
#include <jemalloc/jemalloc.h>
#include <type_traits>


namespace dsk{


inline constexpr auto _je_aligned_alloc = [](align_val_t align, size_t size) noexcept
{
    return je_aligned_alloc(static_cast<size_t>(align), size);
};

template<class T>
struct je_allocator : min_aligned_allocator_base<T, _je_aligned_alloc, je_free>
{
    constexpr je_allocator() = default;

    template<class U>
    constexpr je_allocator(je_allocator<U> const&) noexcept
    {}

    template<class U>
    constexpr bool operator==(je_allocator<U> const&) const noexcept
    {
        return true;
    }
};


struct je_c_alloc
{
#ifdef _MSC_VER // dllimport-ed function cannot be used as constexpr
    inline static const auto malloc  = je_malloc;
    inline static const auto calloc  = je_calloc;
    inline static const auto realloc = je_realloc;
    inline static const auto free    = je_free;
    inline static const auto aligned_alloc = je_aligned_alloc;
#else
    static constexpr auto malloc  = je_malloc;
    static constexpr auto calloc  = je_calloc;
    static constexpr auto realloc = je_realloc;
    static constexpr auto free    = je_free;
    static constexpr auto aligned_alloc = je_aligned_alloc;
#endif
};


} // namespace dsk
