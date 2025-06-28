#pragma once

#if    !defined(DSK_USE_STD_UNORDERED)     \
    && !defined(DSK_USE_BOOST_UNORDERED)

    #if __has_include(<boost/unordered/unordered_node_map.hpp>)

        #define DSK_USE_BOOST_UNORDERED

    #else

        #define DSK_USE_STD_UNORDERED

    #endif

#endif


#include <dsk/default_allocator.hpp>
#include <dsk/util/hash.hpp>
#include <dsk/util/allocator.hpp>
#include <dsk/util/heterogeneous_comparator.hpp>


// stable_unordered_xxx: reference/pointer to value_type is stable, no move of value_type required.
// unstable_unordered_xxx: reference/pointer to value_type will be invalidated when rehashing happens, move of value_type required.
//                         But some operations may be faster than stable version.
// NOTE: the allocator argument is right after K/M, and it will do the rebind before passing down.

#if defined(DSK_USE_STD_UNORDERED)

    #include <unordered_map>
    #include <unordered_set>

    namespace dsk
    {
        template<class T>
        using default_hash = std_hash_for<T>;

        template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using stable_unordered_map = std::unordered_map<K, M, Hash, Eq, rebind_alloc<Al, std::pair<K const, M>>>;

        template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using stable_unordered_set = std::unordered_set<K, Hash, Eq, rebind_alloc<Al, K>>;


        template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using unstable_unordered_map = std::unordered_map<K, M, Hash, Eq, rebind_alloc<Al, std::pair<K const, M>>>;

        template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using unstable_unordered_set = std::unordered_set<K, Hash, Eq, rebind_alloc<Al, K>>;
    }

#endif


#if defined(DSK_USE_BOOST_UNORDERED)

    #include <boost/unordered/unordered_node_map.hpp>
    #include <boost/unordered/unordered_node_set.hpp>
    #include <boost/unordered/unordered_flat_map.hpp>
    #include <boost/unordered/unordered_flat_set.hpp>

    namespace dsk
    {
        template<class T>
        using default_hash = boost_hash_for<T>;

        template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using stable_unordered_map = boost::unordered_node_map<K, M, Hash, Eq, rebind_alloc<Al, std::pair<K const, M>>>;

        template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using stable_unordered_set = boost::unordered_node_set<K, Hash, Eq, rebind_alloc<Al, K>>;


        template<class K, class M, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using unstable_unordered_map = boost::unordered_flat_map<K, M, Hash, Eq, rebind_alloc<Al, std::pair<K const, M>>>;

        template<class K, class Al = DSK_DEFAULT_ALLOCATOR<void>, class Hash = default_hash<K>, class Eq = equal_to<K>>
        using unstable_unordered_set = boost::unordered_flat_set<K, Hash, Eq, rebind_alloc<Al, K>>;
    }

#endif
