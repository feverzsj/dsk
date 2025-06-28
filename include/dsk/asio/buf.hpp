#pragma once

#include <dsk/util/buf.hpp>
#include <dsk/asio/config.hpp>
#include <boost/asio/buffer.hpp>


namespace dsk{


auto asio_buf(_borrowed_byte_buf_ auto&& b) noexcept
{
    return asio::buffer(buf_data(b), buf_size(b));
}

auto asio_buf(_borrowed_byte_buf_ auto&& b, size_t n) noexcept
{
    DSK_ASSERT(buf_size(b) >= n);

    return asio::buffer(buf_data(b), n);
}

auto asio_buf(auto* b, size_t n) noexcept
{
    return asio::buffer(b, n);
}


auto buy_asio_buf(_resizable_byte_buf_ auto& b, size_t n) noexcept
{
    return asio::buffer(buy_buf(b, n), n);
}


} // namespace dsk
