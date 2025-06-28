#pragma once

#include <dsk/default_allocator.hpp>
#include <dsk/util/heterogeneous_comparator.hpp>
#include <dsk/util/ordered_unique_range.hpp>

#include <boost/container/flat_map.hpp>


namespace dsk
{
    template<class K, class M, class ContOrAl = DSK_DEFAULT_ALLOCATOR<std::pair<K, M>>, class Comp = less<K>>
    using flat_map = boost::container::flat_map<K, M, Comp, ContOrAl>;

    template<class K, class M, class ContOrAl = DSK_DEFAULT_ALLOCATOR<std::pair<K, M>>, class Comp = less<K>>
    using flat_multimap = boost::container::flat_multimap<K, M, Comp, ContOrAl>;
}

