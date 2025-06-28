#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/read_at.hpp>


namespace dsk{


auto read_at(auto& d, uint64_t offset, _borrowed_byte_buf_ auto&& b, auto&&... compCond)
{
    return asio::async_read_at(d, offset, asio_buf(DSK_FORWARD(b)), DSK_FORWARD(compCond)..., use_async_op);
}

auto read_at(auto& d, uint64_t offset, auto&& b, auto&&... compCond)
{
    return asio::async_read_at(d, offset, DSK_FORWARD(b), DSK_FORWARD(compCond)..., use_async_op);
}


} // namespace dsk
