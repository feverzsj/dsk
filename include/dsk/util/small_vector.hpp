#pragma once

#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <boost/container/small_vector.hpp>


namespace dsk
{
    template<class T, size_t N, class Al = DSK_DEFAULT_ALLOCATOR<T>, class Opts = void>
    using small_vector = boost::container::small_vector<T, N, rebind_alloc<Al, T>, Opts>;
}
