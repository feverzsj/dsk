#pragma once

#include <dsk/config.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <memory>
#include <type_traits>


namespace dsk{


template<class T, class Alloc>
struct allocate_unique_maker
{
    using    traits = rebind_traits<Alloc, T>;
    using allocator = traits::allocator_type;
    using   pointer = traits::pointer;

    struct deleter
    {
        DSK_NO_UNIQUE_ADDR allocator a;

        constexpr void operator()(pointer p)
        {
            traits::destroy(a, p);
            traits::deallocate(a, p, 1);
        }
    };

    using unique_ptr_type = std::unique_ptr<T, deleter>;

    DSK_NO_UNIQUE_ADDR allocator a;
    pointer p;

    constexpr explicit allocate_unique_maker(auto&& a_)
        : a(a_), p(traits::allocate(a, 1))
    {}

    constexpr ~allocate_unique_maker()
    {
        if(p)
            traits::deallocate(a, p, 1);
    }

    constexpr auto release(auto&&... args) noexcept
    {
        traits::construct(a, p, DSK_FORWARD(args)...);

        return unique_ptr_type(std::exchange(p, pointer()), deleter(a));
    }
};


template<class T, class Alloc>
using allocated_unique_ptr = typename allocate_unique_maker<T, Alloc>::unique_ptr_type;


template<class T, class Alloc>
constexpr auto allocate_unique(Alloc const& a, auto&&... args) requires(! std::is_array_v<T>)
{
    return allocate_unique_maker<T, Alloc>(a).release(DSK_FORWARD(args)...);
}

constexpr auto allocate_unique(auto const& a, auto&& v)
{
    return allocate_unique<DSK_DECAY_T(v)>(a, DSK_FORWARD(v));
}


template<class T>
using default_allocated_unique_ptr = allocated_unique_ptr<T, DSK_DEFAULT_ALLOCATOR<T>>;

template<class T>
constexpr auto default_allocate_unique(auto&&... args) requires(! std::is_array_v<T>)
{
    return allocate_unique<T>(DSK_DEFAULT_ALLOCATOR<T>(), DSK_FORWARD(args)...);
}


} // namespace dsk
