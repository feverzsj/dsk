#pragma once

#include <dsk/scheduler.hpp>
#include <dsk/continuation.hpp>
#include <type_traits>


namespace dsk{


template<class T>
concept _resumer_ = requires{ typename std::remove_cvref_t<T>::is_resumer; };

template<class T>
concept _scheduler_or_resumer_ = _scheduler_<T> || _resumer_<T>;


template<_scheduler_ Sch>
class scheudler_resumer
{
    Sch* _sch = nullptr;

public:
    constexpr scheudler_resumer(Sch& s) : _sch(std::addressof(s)) {}

    using is_resumer = void;

    constexpr auto& scheduler(this auto& self) noexcept { return *self._sch; }
    constexpr auto* typed_id() const noexcept { return _sch; }

    constexpr void post(_continuation_ auto&& cont)
    {
        _sch->post(DSK_FORWARD(cont));
    }
};

template<class Sch>
scheudler_resumer(Sch) -> scheudler_resumer<Sch>;


constexpr decltype(auto) get_resumer(_scheduler_ auto& sch) noexcept
{
    if constexpr(requires{ sch.get_resumer(); }) return sch.get_resumer();
    else                                         return scheudler_resumer(sch);
}

constexpr auto&& get_resumer(_resumer_ auto&& r) noexcept
{
    return DSK_FORWARD(r);
}

constexpr decltype(auto) get_resumer_or_fwd(auto&& t) noexcept
{
    if constexpr(_scheduler_or_resumer_<decltype(t)>) return get_resumer(DSK_FORWARD(t));
    else                                              return DSK_FORWARD(t);
}


constexpr auto* get_typed_id(_scheduler_ auto& sch) noexcept
{
    return std::addressof(sch);
}

constexpr auto* get_typed_id(_resumer_ auto&& r) noexcept
{
    return r.typed_id();
}


constexpr bool operator==(_scheduler_or_resumer_ auto const& l, _scheduler_or_resumer_ auto const& r) noexcept
{
    if constexpr(requires{ get_typed_id(l) == get_typed_id(r); })
    {
        return get_typed_id(l) == get_typed_id(r);
    }
    else
    {
        return false;
    }
}


template<class T>
concept _resumer_aware_continuation_ = requires{ typename std::remove_cvref_t<T>::is_resumer_aware_continuation; }
                                       && _continuation_<T>;

constexpr auto&& get_continuation(_continuation_ auto&& cont) noexcept
{
    if constexpr(_resumer_aware_continuation_<decltype(cont)>) return DSK_FORWARD(cont).get_continuation();
    else                                                       return DSK_FORWARD(cont);
}


constexpr void resume(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr)
{
    if constexpr(_resumer_aware_continuation_<decltype(cont)>)
    {
        DSK_FORWARD(cont)();
    }
    else
    {
        sr.post(DSK_FORWARD(cont));
    }
}

constexpr void resume(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr, auto* curSrId)
{
    if constexpr(_resumer_aware_continuation_<decltype(cont)>)
    {
        DSK_FORWARD(cont)();
    }
    else
    {
        if constexpr(requires{ get_typed_id(sr) == curSrId; })
        {
            if(get_typed_id(sr) == curSrId)
            {
                DSK_FORWARD(cont)(); // directly invoke cont, if already on curSr
                return;
            }
        }

        sr.post(DSK_FORWARD(cont));
    }
}

constexpr void resume(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr, _scheduler_or_resumer_ auto&& curSr)
{
    resume(DSK_FORWARD(cont), DSK_FORWARD(sr), get_typed_id(curSr));
}


template<class Resumer, class Cont>
struct resumer_aware_continuation
{
    static_assert(! _resumer_aware_continuation_<Cont>);

    DSK_NO_UNIQUE_ADDR Resumer _resumer;
    DSK_NO_UNIQUE_ADDR Cont    _cont;

    using is_resumer_aware_continuation = void;

    constexpr auto&& get_resumer     (this auto&& self) noexcept { return DSK_FORWARD(self)._resumer; }
    constexpr auto&& get_continuation(this auto&& self) noexcept { return DSK_FORWARD(self)._cont; }

    constexpr void operator()() { _resumer.post(mut_move(_cont)); }
};


constexpr auto bind_resumer(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr)
{
    return make_templated<resumer_aware_continuation>(get_resumer(DSK_FORWARD(sr)),
                                                      get_continuation(DSK_FORWARD(cont)));
}

template<bool Cond>
constexpr decltype(auto) bind_resumer_if(_continuation_ auto&& cont, _scheduler_or_resumer_ auto&& sr)
{
    if constexpr(Cond) return bind_resumer(DSK_FORWARD(cont), DSK_FORWARD(sr));
    else               return DSK_FORWARD(cont);
}


} // namespace dsk
