#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/buf.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/write.hpp>


namespace dsk{


auto write(auto& s, _borrowed_byte_buf_ auto&& b, auto&&... compCond)
{
    return asio::async_write(s, asio_buf(DSK_FORWARD(b)), DSK_FORWARD(compCond)..., use_async_op);
}

auto write(auto& s, auto&& b, auto&&... compCond)
{
    return asio::async_write(s, DSK_FORWARD(b), DSK_FORWARD(compCond)..., use_async_op);
}


} // namespace dsk
