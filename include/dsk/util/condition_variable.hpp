#pragma once

#include <dsk/util/thread_etc_config.hpp>


#if defined(DSK_USE_STD_TRHEAD_ETC)

    #include <condition_variable>

    namespace dsk
    {
        using std::condition_variable;
        using std::condition_variable_any;
        using std::notify_all_at_thread_exit;
        using std::cv_status;
    }

#endif


#if defined(DSK_USE_BOOST_TRHEAD_ETC)

    #include <boost/thread/condition_variable.hpp>

    namespace dsk
    {
        using boost::condition_variable;
        using boost::condition_variable_any;
        using boost::notify_all_at_thread_exit;
        using boost::cv_status;
    }

#endif
