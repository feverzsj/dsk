#include <dsk/util/unit.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/util/periodic_reporter.hpp>
#include <chrono>
#include <cstdio>


namespace dsk{


template<
    class CntDur,
    class CntUnitDur  = CntDur,
    class TimeUnitDur = std::chrono::seconds,
    auto  Report      = stdout_report,
    class Clock       = std::chrono::high_resolution_clock,
    bool  Lock        = false
>
class perf_counter
{
    using clock_tp  = typename Clock::time_point;
    using clock_dur = typename Clock::duration;
    using cnt_unit_dur  = std::chrono::duration<double, typename CntUnitDur::period>;
    using time_unit_dur = std::chrono::duration<double, typename TimeUnitDur::period>;

    string _name, _cntUnit, _timeUnit = "s";

    periodic_reporter_t<Report, Clock> _reporter;
    CntDur    _prevCnt = CntDur::zero();
    clock_dur _prevDur = clock_dur::zero();

    CntDur    _totalCnt = CntDur::zero();
    clock_dur _totalDur = clock_dur::zero();

    DSK_DEF_MEMBER_IF(Lock, mutex) _mtx;

public:
    perf_counter() = default;

    perf_counter(_byte_str_ auto&& name, _byte_str_ auto&& cntUnit)
    {
        set_name    (DSK_FORWARD(name));
        set_cnt_unit(DSK_FORWARD(cntUnit));
    }

    void set_name     (_byte_str_ auto&& s) { assign_str(_name    , DSK_FORWARD(s)); }
    void set_cnt_unit (_byte_str_ auto&& s) { assign_str(_cntUnit , DSK_FORWARD(s)); }
    void set_time_unit(_byte_str_ auto&& s) { assign_str(_timeUnit, DSK_FORWARD(s)); }

    void set_report_interval(clock_dur const& d)
    {
        _reporter.set_interval(d);
    }

    void restart()
    {
        conditional_lock_guard<Lock> lg(_mtx);
        
        _reporter.restart();
        _prevCnt = CntDur::zero();
        _prevDur = clock_dur::zero();
        _totalCnt = CntDur::zero();
        _totalDur = clock_dur::zero();
    }

    void add(CntDur const& cnt, clock_dur const& dur)
    {
        conditional_lock_guard<Lock> lg(_mtx);

        _totalCnt += cnt;
        _totalDur += dur;

        bool reported = _reporter.report([&]()
        {
            auto cur     = cnt_unit_dur(_prevCnt + cnt).count() / time_unit_dur(_prevDur + dur).count();
            auto average = cnt_unit_dur(_totalCnt).count() / time_unit_dur(_totalDur).count();
            auto total   = cnt_unit_dur(_totalCnt).count();

            return cat_as_str(
                _name,
                ": ", with_fmt<'f', 2>(cur), ' ', _cntUnit, '/', _timeUnit,
                ", Average: ", with_fmt<'f', 2>(average), ' ', _cntUnit, '/', _timeUnit,
                ", Total: ", with_fmt<'f', 2>(total), ' ', _cntUnit,
                '\n'
            );
        });

        if(reported)
        {
            _prevCnt = CntDur::zero();
            _prevDur = clock_dur::zero();
        }
        else
        {
            _prevCnt += cnt;
            _prevDur += dur;
        }
    }

    struct count_scope
    {
        perf_counter& self;
        CntDur   cnt = CntDur::zero();
        clock_tp beg = Clock::now();
        bool     ended = false;

        ~count_scope()
        {
            if(! ended)
            {
                end();
            }
        }

        void end()
        {
            DSK_ASSERT(! ended);
            self.add(cnt, Clock::now() - beg);
            ended = true;
        }

        void add(CntDur const& d) noexcept
        {
            cnt += d;
        }

        void add_and_end(CntDur const& d)
        {
            add(d);
            end();
        }
    };
    
    count_scope begin_count_scope() noexcept
    {
        return {*this};
    }
};


template<
    class CntDur,
    class CntUnitDur  = CntDur,
    class TimeUnitDur = std::chrono::seconds,
    auto  Report      = stdout_report,
    class Clock       = std::chrono::high_resolution_clock
>
using mt_perf_counter = perf_counter<CntDur, CntUnitDur, TimeUnitDur, Report, Clock, true>;


} // namespace dsk
