#pragma once

#include <dsk/util/allocator.hpp>
#include <tbb/scalable_allocator.h>
#include <type_traits>


namespace dsk{


// if define the lambda directly in template, each time use it, a different identity will be initialized.
inline constexpr auto _tbb_scalable_aligned_alloc = [](align_val_t align, size_t size) noexcept
{
    return scalable_aligned_malloc(size, static_cast<size_t>(align));
};

// tbb::scalable_allocator can't hanlde alignof(T) > sizeof(void*), so we made this allocator.
template<class T>
struct tbb_scalable_allocator : min_aligned_allocator_base<T, _tbb_scalable_aligned_alloc, scalable_free>
{
    constexpr tbb_scalable_allocator() = default;

    template<class U>
    constexpr tbb_scalable_allocator(tbb_scalable_allocator<U> const&) noexcept
    {}

    template<class U>
    constexpr bool operator==(tbb_scalable_allocator<U> const&) const noexcept
    {
        return true;
    }
};


struct tbb_scalable_c_alloc
{
#ifdef _MSC_VER // dllimport-ed function cannot be used as constexpr
    inline static const auto malloc  = scalable_malloc;
    inline static const auto calloc  = scalable_calloc;
    inline static const auto realloc = scalable_realloc;
    inline static const auto free    = scalable_free;
#else
    static constexpr auto malloc  = scalable_malloc;
    static constexpr auto calloc  = scalable_calloc;
    static constexpr auto realloc = scalable_realloc;
    static constexpr auto free    = scalable_free;
#endif
    static constexpr auto aligned_alloc = [](size_t align, size_t size) noexcept { return scalable_aligned_malloc(size, align); };
};


} // namespace dsk
