#pragma once


#if !defined(DSK_DEFAULT_IO_SCHEDULER) && !defined(DSK_DEFAULT_IO_SCHEDULER_INCLUDE)
    
    #include <dsk/asio/thread_pool.hpp>

    namespace dsk
    {
        inline auto& get_default_io_scheduler()
        {
            static asio_io_thread_pool ioSch;
            return ioSch;
        }
    }

    #define DSK_DEFAULT_IO_SCHEDULER ::dsk::get_default_io_scheduler()

#endif


#if defined(DSK_DEFAULT_IO_SCHEDULER_INCLUDE)
    #include DSK_DEFAULT_IO_SCHEDULER_INCLUDE
#endif

#if !defined(DSK_DEFAULT_IO_SCHEDULER)
    #error DSK_DEFAULT_IO_SCHEDULER not defined
#endif

#if !defined(DSK_DEFAULT_IO_CONTEXT)
    #define DSK_DEFAULT_IO_CONTEXT DSK_DEFAULT_IO_SCHEDULER.context()
#endif


#include <dsk/resumer.hpp>

namespace dsk{


class default_io_scheduler_resumer
{
public:
    using is_resumer = void;

    static constexpr auto& scheduler() noexcept { return DSK_DEFAULT_IO_SCHEDULER; }
    static constexpr auto* typed_id() noexcept { return std::addressof(scheduler()); }

    constexpr void post(_continuation_ auto&& cont)
    {
        scheduler().post(DSK_FORWARD(cont));
    }
};


} // namespace dsk
