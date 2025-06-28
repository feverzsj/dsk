#pragma once

#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <deque>


namespace dsk
{
    template<class T, class Al = DSK_DEFAULT_ALLOCATOR<T>>
    using deque = std::deque<T, rebind_alloc<Al, T>>;
}
