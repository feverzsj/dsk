#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/connect.hpp>


namespace dsk{


auto connect(auto& socket, auto&& endpoints, auto&&... connCond)
{
    return asio::async_connect(socket, DSK_FORWARD(endpoints), DSK_FORWARD(connCond)..., use_async_op);
}


} // namespace dsk
