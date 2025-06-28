#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/write_at.hpp>


namespace dsk{


auto write_at(auto& d, uint64_t offset, _borrowed_byte_buf_ auto&& b, auto&&... compCond)
{
    return asio::async_write_at(d, offset, asio_buf(DSK_FORWARD(b)), DSK_FORWARD(compCond)..., use_async_op);
}

auto write_at(auto& d, uint64_t offset, auto&& b, auto&&... compCond)
{
    return asio::async_write_at(d, offset, DSK_FORWARD(b), DSK_FORWARD(compCond)..., use_async_op);
}


} // namespace dsk
