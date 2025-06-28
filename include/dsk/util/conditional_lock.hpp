#pragma once

#include <dsk/util/mutex.hpp>
#include <dsk/util/type_traits.hpp>


namespace dsk{


template<bool Cond, class Mutex = mutex>
using conditional_lock_guard = DSK_DECL_TYPE_IF(Cond, lock_guard<Mutex>);


struct dummy_unique_lock
{
    constexpr dummy_unique_lock(dummy_unique_lock&&) = default;
    constexpr dummy_unique_lock(auto&&...) noexcept {}
    constexpr void unlock() noexcept {}
};

template<bool Cond, class Mutex = mutex>
using conditional_unique_lock = std::conditional_t<Cond, unique_lock<Mutex>,
                                                         dummy_unique_lock>;


} // namespace dsk
