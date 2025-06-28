#pragma once

#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <list>


namespace dsk
{
    template<class T, class Al = DSK_DEFAULT_ALLOCATOR<T>>
    using list = std::list<T, rebind_alloc<Al, T>>;
}
