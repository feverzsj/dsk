#pragma once

#include <dsk/default_allocator.hpp>
#include <dsk/util/heterogeneous_comparator.hpp>
#include <dsk/util/ordered_unique_range.hpp>

#include <boost/container/flat_set.hpp>


namespace dsk
{
    template<class K, class ContOrAl = DSK_DEFAULT_ALLOCATOR<K>, class Comp = less<K>>
    using flat_set = boost::container::flat_set<K, Comp, ContOrAl>;

    template<class K, class ContOrAl = DSK_DEFAULT_ALLOCATOR<K>, class Comp = less<K>>
    using flat_multiset = boost::container::flat_multiset<K, Comp, ContOrAl>;
}

