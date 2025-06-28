#pragma once

#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <vector>


namespace dsk
{
    template<class T, class Al = DSK_DEFAULT_ALLOCATOR<T>>
    using vector = std::vector<T, rebind_alloc<Al, T>>;
}
