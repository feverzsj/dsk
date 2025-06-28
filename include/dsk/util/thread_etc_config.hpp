#pragma once


#if    !defined(DSK_USE_STD_TRHEAD_ETC)     \
    && !defined(DSK_USE_BOOST_TRHEAD_ETC)

//     #if __has_include(<boost/thread.hpp>)
// 
//         #define DSK_USE_BOOST_TRHEAD_ETC
// 
//     #else

        #define DSK_USE_STD_TRHEAD_ETC

//     #endif

#endif
