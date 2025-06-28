#pragma once

#include <dsk/default_allocator.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/util/heterogeneous_comparator.hpp>
#include <map>


namespace dsk
{
    template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Comp = less<K>>
    using map = std::map<K, M, Comp, rebind_alloc<Al, std::pair<K const, M>>>;

    template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Comp = less<K>>
    using multimap = std::multimap<K, M, Comp, rebind_alloc<Al, std::pair<K const, M>>>;
}

