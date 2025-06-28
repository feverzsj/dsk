#pragma once

#include <dsk/util/thread_etc_config.hpp>


#if defined(DSK_USE_STD_TRHEAD_ETC)

    #include <thread>

    namespace dsk
    {
        using std::thread;
        namespace this_thread = std::this_thread;
    }

#endif


#if defined(DSK_USE_BOOST_TRHEAD_ETC)

    #include <boost/thread/thread.hpp>

    namespace dsk
    {
        using boost::thread;
        namespace this_thread = boost::this_thread;
    }

#endif
