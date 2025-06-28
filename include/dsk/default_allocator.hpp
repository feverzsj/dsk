#pragma once


// NOTE: DSK_DEFAULT_ALLOCATOR must be stateless, as there is no way to assign it everywhere.
// NOTE: TBB/TBB_CACHE_ALIGNED are not align aware, so they may not handle over-aligned allocation properly.

#if defined(DSK_DEFAULT_ALLOCATOR_INCLUDE)

    #include DSK_DEFAULT_ALLOCATOR_INCLUDE

#elif !defined(DSK_DEFAULT_ALLOCATOR) && !defined(DSK_DEFAULT_C_ALLOC)

    #if defined(DSK_DEFAULT_ALLOCATOR_USE_MI)

        #include <dsk/util/mimalloc.hpp>
        #define DSK_DEFAULT_ALLOCATOR ::dsk::mi_allocator
        #define DSK_DEFAULT_C_ALLOC ::dsk::mi_c_alloc

    #elif defined(DSK_DEFAULT_ALLOCATOR_USE_TBB)

        #include <tbb/tbb_allocator.h>
        #define DSK_DEFAULT_ALLOCATOR tbb::tbb_allocator
        #include <dsk/tbb/allocator.hpp>
        #define DSK_DEFAULT_C_ALLOC ::dsk::tbb_scalable_c_alloc

    #elif defined(DSK_DEFAULT_ALLOCATOR_USE_TBB_CACHE_ALIGNED)

        #include <tbb/cache_aligned_allocator.h>
        #define DSK_DEFAULT_ALLOCATOR tbb::cache_aligned_allocator
        #include <dsk/tbb/allocator.hpp>
        #define DSK_DEFAULT_C_ALLOC ::dsk::tbb_scalable_c_alloc

    #elif defined(DSK_DEFAULT_ALLOCATOR_USE_TBB_SCALABLE)

        #include <dsk/tbb/allocator.hpp>
        #define DSK_DEFAULT_ALLOCATOR ::dsk::tbb_scalable_allocator
        #define DSK_DEFAULT_C_ALLOC ::dsk::tbb_scalable_c_alloc

    #elif defined(DSK_DEFAULT_ALLOCATOR_USE_JE)

        #include <dsk/util/jemalloc.hpp>
        #define DSK_DEFAULT_ALLOCATOR ::dsk::je_allocator
        #define DSK_DEFAULT_C_ALLOC ::dsk::je_c_alloc

    #else

        #include <dsk/util/allocator.hpp>
        #define DSK_DEFAULT_ALLOCATOR std::allocator
        #define DSK_DEFAULT_C_ALLOC ::dsk::std_c_alloc

    #endif

#endif



#if !defined(DSK_DEFAULT_ALLOCATOR)
    #error DSK_DEFAULT_ALLOCATOR not defined
#endif

#if !defined(DSK_DEFAULT_C_ALLOC)
    #error DSK_DEFAULT_C_ALLOC not defined
#endif
