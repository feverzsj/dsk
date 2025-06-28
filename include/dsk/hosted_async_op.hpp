#pragma once

#include <dsk/async_op.hpp>
#include <dsk/util/allocate_unique.hpp>


namespace dsk{


template<class Op, _unique_ptr_ HostPtr>
struct hosted_async_op : Op
{
    HostPtr hostPtr;
};


template<class Host, class Alloc = DSK_DEFAULT_ALLOCATOR<Host>>
constexpr auto make_hosted_async_op(auto&& genOp, auto&&... hostArgs)
{
    auto hostPtr = allocate_unique<Host>(Alloc(), DSK_FORWARD(hostArgs)...);

    return hosted_async_op<
        decltype(genOp(std::declval<Host&>())), decltype(hostPtr)
    >{{genOp(*hostPtr)}, mut_move(hostPtr)};
}


} // namespace dsk
