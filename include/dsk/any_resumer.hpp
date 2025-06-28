#pragma once

#include <dsk/resumer.hpp>
#include <dsk/inline_scheduler.hpp>


namespace dsk{


class any_resumer
{
    void* _sch = nullptr;
    void (*_post)(void*, continuation&&) = nullptr;

    static continuation&& to_cont(continuation&& cont) noexcept { return mut_move(cont); }
    static continuation to_cont(auto&& cont) { return DSK_FORWARD(cont); }

public:
    any_resumer() = default;
    any_resumer(any_resumer const&) = default;
    any_resumer& operator=(any_resumer const&) = default;

    any_resumer(inline_scheduler) noexcept {}
    any_resumer(inline_resumer) noexcept {}

    any_resumer(_scheduler_ auto& sch) noexcept
        : _sch(std::addressof(sch))
        , _post([](void* s, continuation&& c)
          {
              static_cast<DSK_NO_REF_T(sch)*>(s)->post(mut_move(c));
          })
    {}

    any_resumer(_resumer_ auto&& r) noexcept requires(! _no_cvref_same_as_<decltype(r), any_resumer>)
        : any_resumer(r.scheduler())
    {}

    bool is_inline() const noexcept { return ! _sch; }
    void set_inline() noexcept { _sch = nullptr; }

    using is_resumer = void;

    void* typed_id() const noexcept { return _sch; }

    void post(_continuation_ auto&& cont)
    {
        if(is_inline())
        {
            DSK_FORWARD(cont)();
        }
        else
        {
            DSK_ASSERT(_post);
            _post(_sch, to_cont(DSK_FORWARD(cont)));
        }
    }
};


} // namespace dsk
