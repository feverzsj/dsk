#pragma once

#include <dsk/default_allocator.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/util/heterogeneous_comparator.hpp>

#include <set>


namespace dsk
{
    template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Comp = less<K>>
    using set = std::set<K, Comp, rebind_alloc<Al, K>>;

    template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Comp = less<K>>
    using multiset = std::multiset<K, Comp, rebind_alloc<Al, K>>;
}

