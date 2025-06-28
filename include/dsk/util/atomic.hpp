#pragma once


#if    !defined(DSK_USE_STD_ATOMIC)     \
    && !defined(DSK_USE_BOOST_ATOMIC)

//     #if __has_include(<boost/atomic.hpp>)
// 
//         #define DSK_USE_BOOST_ATOMIC
// 
//     #else

        #define DSK_USE_STD_ATOMIC

//     #endif

#endif



#if defined(DSK_USE_STD_ATOMIC)

    #include <atomic>

    namespace dsk
    {
        using std::atomic;
        using std::atomic_ref;
        using std::memory_order;
        using std::memory_order_relaxed;
        //using std::memory_order_consume;
        using std::memory_order_acquire;
        using std::memory_order_release;
        using std::memory_order_acq_rel;
        using std::memory_order_seq_cst;
        using std::atomic_thread_fence;
        using std::atomic_signal_fence;
    }

#endif


#if defined(DSK_USE_BOOST_ATOMIC)

    #include <boost/atomic/atomic.hpp>
    #include <boost/atomic/atomic_ref.hpp>
    #include <boost/atomic/fences.hpp>

    namespace dsk
    {
        using boost::atomic;
        using boost::atomic_ref;
        using boost::memory_order;
        using boost::memory_order_relaxed;
        //using boost::memory_order_consume;
        using boost::memory_order_acquire;
        using boost::memory_order_release;
        using boost::memory_order_acq_rel;
        using boost::memory_order_seq_cst;
        using boost::atomic_thread_fence;
        using boost::atomic_signal_fence;
    }

#endif
