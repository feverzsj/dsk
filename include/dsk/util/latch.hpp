#pragma once

#include <dsk/util/thread_etc_config.hpp>


#if defined(DSK_USE_STD_TRHEAD_ETC)

    #include <latch>

    namespace dsk
    {
        using std::latch;
    }

#endif


#if defined(DSK_USE_BOOST_TRHEAD_ETC)

    #include <dsk/config.hpp>
    #include <dsk/util/debug.hpp>
    #include <boost/thread/latch.hpp>

    namespace dsk
    {

        class latch : private boost::latch
        {
            using base = boost::latch;

        public:
            explicit latch(ptrdiff_t n)
                : base(static_cast<size_t>(n))
            {
                static_assert(max() <= SIZE_MAX);
                DSK_ASSERT(n <= max());
            }

            using base::count_down; // no argument
            using base::try_wait;
            using base::wait;

            void arrive_and_wait() // no argument
            {
                count_down();
                wait();
            }

            static constexpr ptrdiff_t max() noexcept
            {
                return PTRDIFF_MAX;
            }
        };

    } // namespace dsk

#endif
