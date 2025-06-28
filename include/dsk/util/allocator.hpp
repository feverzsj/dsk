#pragma once

#include <dsk/config.hpp>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>
#include <concepts>
#include <type_traits>


namespace dsk{


template<class Alloc, class T>
using rebind_traits = std::allocator_traits<Alloc>::template rebind_traits<T>;

// NOTE: Alloc should not be a type alias.
template<class Alloc, class T>
using rebind_alloc = std::allocator_traits<Alloc>::template rebind_alloc<T>;


template<class T>
inline constexpr bool is_stateless_allocator_v = std::default_initializable<T>
                                                 && std::allocator_traits<T>::is_always_equal::value;
template<class T>
concept _stateless_allocator_ = is_stateless_allocator_v<std::remove_cvref_t<T>>;


using align_val_t = std::align_val_t;


// round up/down size to an integral multiple of alignment(must be power of two).

// NOTE: rounding up could wrap around to zero
constexpr size_t align_up(align_val_t align_, size_t size) noexcept
{
    auto align = static_cast<size_t>(align_);
    return (size + align - 1) & ~(align - 1);
}

constexpr size_t align_down(align_val_t align_, size_t size) noexcept
{
    auto align = static_cast<size_t>(align_);
    return size & ~(align - 1);
}

// works for non power of two alignment
constexpr size_t align_up_any(align_val_t align_, size_t size) noexcept
{
    auto align = static_cast<size_t>(align_);

    if(size_t rem = size % align)
    {
        size += align - rem;
    }

    return size;
}


enum align_up_size_e
{
    align_up_size = true,
    no_align_up_size = false
};

// Make c-style allocator into a c++ allocator.
// Allocated alignment won't less than MinAlign.
template<
    class       T,
    auto        AlignedAlloc,
    auto        Free,
    align_val_t MinAlign = align_val_t(__STDCPP_DEFAULT_NEW_ALIGNMENT__), // suitable for any non over aligned object, also adjustable by compiler flags.
    align_up_size_e AlignUpSize = align_up_size
>
struct min_aligned_allocator_base
{
    static_assert(std::is_invocable_r_v<void*, decltype(AlignedAlloc), align_val_t, size_t>);
    static_assert(std::is_invocable_v<decltype(Free), void*>);
    static_assert(static_cast<size_t>(MinAlign) > 0);
    static_assert(static_cast<size_t>(MinAlign) % 2 == 0);

    static constexpr align_val_t min_align = MinAlign;

    using value_type = T;

    using is_always_equal = std::true_type;

    // false_type will cause allocator aware type to copy content from other when moving.
    using propagate_on_container_move_assignment = std::true_type;

    [[nodiscard]] T* allocate(size_t n)
    {
        if(n == 0)
        {
            return nullptr;
        }

        if constexpr(sizeof(T) > 1)
        {
            if(n > static_cast<size_t>(-1) / sizeof(T))
            {
                throw std::bad_array_new_length();
            }
        }

        size_t          size = n * sizeof(T);
        constexpr auto align = align_val_t(std::max(alignof(T), static_cast<size_t>(MinAlign)));

        if constexpr(AlignUpSize)
        {
            size_t rs = align_up(align, size);
            
            if(rs < size)
            {
                throw std::bad_array_new_length();
            }

            size = rs;
        }
        
        void* p = AlignedAlloc(align, size);

        if(! p)
        {
            throw std::bad_alloc();
        }

        return static_cast<T*>(p);
    }

    void deallocate(T* p, size_t) noexcept
    {
        Free(p);
    }
};


struct std_c_alloc
{
#ifdef _MSC_VER // dllimport-ed function cannot be used as constexpr
    inline static const auto malloc  = std::malloc;
    inline static const auto calloc  = std::calloc;
    inline static const auto realloc = std::realloc;
    inline static const auto free    = std::free;
#else
    static constexpr auto malloc  = std::malloc;
    static constexpr auto calloc  = std::calloc;
    static constexpr auto realloc = std::realloc;
    static constexpr auto free    = std::free;
    static constexpr auto aligned_alloc = std::aligned_alloc;
#endif
};


} // namespace dsk
