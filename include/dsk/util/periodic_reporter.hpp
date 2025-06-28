#include <dsk/util/str.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/conditional_lock.hpp>
#include <chrono>
#include <cstdio>


namespace dsk{


struct stdout_report_t
{
    void operator()(_byte_str_ auto const& s) const
    {
        if(size_t n = str_size(s))
        {
            fwrite(str_data(s), 1, n, stdout);
            //fputc('\n', stdout);
            fflush(stdout);
        }
    }

    void operator()(_stringifible_ auto const&... ss) const
    {
        operator()(cat_as_or_fwd_str(ss...));
    }
};

inline constexpr stdout_report_t stdout_report;


template<
    auto  Report = stdout_report,
    class Clock  = std::chrono::high_resolution_clock,
    bool  Lock   = false
>
class periodic_reporter_t
{
    using clock_tp  = typename Clock::time_point;
    using clock_dur = typename Clock::duration;

    clock_dur _intv = std::chrono::seconds(10);
    clock_tp  _prev = Clock::now();

    DSK_DEF_MEMBER_IF(Lock, mutex) _mtx;

public:
    periodic_reporter_t() = default;

    explicit periodic_reporter_t(clock_dur const& d)
        : _intv(d)
    {}

    void set_interval(clock_dur const& d)
    {
        _intv = d;
    }

    void restart()
    {
        conditional_lock_guard<Lock> lg(_mtx);
        _prev = Clock::now();
    }

    bool report(_nullary_callable_ auto&& gen)
    {
        conditional_unique_lock<Lock> lk(_mtx);

        auto now = Clock::now();

        if(now - _prev >= _intv)
        {
            _prev = now;

            lk.unlock();
            Report(gen());
            return true;
        }

        return false;
    }
};


template<
    auto  Report = stdout_report,
    class Clock  = std::chrono::high_resolution_clock
>
using mt_periodic_reporter_t = periodic_reporter_t<Report, Clock, true>;


using periodic_reporter = periodic_reporter_t<>;
using mt_periodic_reporter = mt_periodic_reporter_t<>;


} // namespace dsk
