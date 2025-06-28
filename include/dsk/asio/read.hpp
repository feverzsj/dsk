#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/buf.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/read.hpp>


namespace dsk{


auto read(auto& s, _borrowed_byte_buf_ auto&& b, auto&&... compCond)
{
    return asio::async_read(s, asio_buf(DSK_FORWARD(b)), DSK_FORWARD(compCond)..., use_async_op);
}

auto read(auto& s, auto&& b, auto&&... compCond)
{
    return asio::async_read(s, DSK_FORWARD(b), DSK_FORWARD(compCond)..., use_async_op);
}


} // namespace dsk
