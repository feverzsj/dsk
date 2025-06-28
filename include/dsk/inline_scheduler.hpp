#pragma once

#include <dsk/config.hpp>
#include <dsk/resumer.hpp>


namespace dsk{


struct inline_scheduler
{
    using is_scheduler = void;

    static constexpr void post(_continuation_ auto&& cont) { DSK_FORWARD(cont)(); }
    static constexpr auto get_resumer() noexcept;
};


inline constexpr inline_scheduler g_inline_scheculer;


class inline_resumer
{
public:
    using is_resumer = void;

    static constexpr auto& scheduler() noexcept { return g_inline_scheculer; }
    static constexpr inline_scheduler* typed_id() noexcept { return nullptr; }

    constexpr void post(_continuation_ auto&& cont)
    {
        DSK_FORWARD(cont)();
    }
};


constexpr auto inline_scheduler::get_resumer() noexcept
{
    return inline_resumer();
}


constexpr bool is_inline(_scheduler_or_resumer_ auto&& sr) noexcept
{
    return get_resumer(DSK_FORWARD(sr)) == inline_resumer();
}


} // namespace dsk
