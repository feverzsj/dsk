#pragma once

#include <dsk/util/thread_etc_config.hpp>


#if defined(DSK_USE_STD_TRHEAD_ETC)

    #include <mutex>

    namespace dsk
    {
        using std::mutex;
        using std::timed_mutex;
        using std::recursive_mutex;
        using std::recursive_timed_mutex;
        using std::lock_guard;
        using std::scoped_lock;
        using std::unique_lock;
        using std::defer_lock_t;
        using std::try_to_lock_t;
        using std::adopt_lock_t;
        using std::defer_lock;
        using std::try_to_lock;
        using std::adopt_lock;
        using std::try_lock;
        using std::lock;
    }

#endif


#if defined(DSK_USE_BOOST_TRHEAD_ETC)

    #include <dsk/util/tuple.hpp>
    #include <boost/thread/mutex.hpp>
    #include <boost/thread/recursive_mutex.hpp>
    #include <boost/thread/lock_guard.hpp>
    #include <boost/thread/lock_types.hpp>
    #include <boost/thread/lock_algorithms.hpp>

    namespace dsk
    {

        using boost::mutex;
        using boost::timed_mutex;
        using boost::recursive_mutex;
        using boost::recursive_timed_mutex;
        using boost::lock_guard;
        using boost::unique_lock;
        using boost::defer_lock_t;
        using boost::try_to_lock_t;
        using boost::adopt_lock_t;
        using boost::defer_lock;
        using boost::try_to_lock;
        using boost::adopt_lock;
        using boost::try_lock;
        using boost::lock;

        template<class... Mutexes>
        class [[nodiscard]] scoped_lock
        {
        public:
            explicit scoped_lock(Mutexes&... mtxes)
                : _mtxes(mtxes...)
            {
                dsk::lock(mtxes...);
            }

            explicit scoped_lock(adopt_lock_t, Mutexes&... mtxes) noexcept
                : _mtxes(mtxes...)
            {}

            ~scoped_lock() noexcept
            {
                foreach_elms(_mtxes, [](auto& mtx){ mtx.unlock(); });
            }

            scoped_lock(const scoped_lock&) = delete;
            scoped_lock& operator=(const scoped_lock&) = delete;

        private:
            tuple<Mutexes&...> _mtxes;
        };


    } // namespace dsk

#endif
