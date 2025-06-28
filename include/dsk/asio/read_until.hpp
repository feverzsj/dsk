#pragma once

#include <dsk/asio/config.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/read_until.hpp>


namespace dsk{


auto read_until(auto& s, _resizable_byte_buf_ auto& b, auto&& match)
{
    return asio::async_read_until(s, asio::dynamic_buffer(b), DSK_FORWARD(match), use_async_op);
}

template<class Al>
auto read_until(auto& s,  boost::asio::basic_streambuf<Al>& b, auto&& match)
{
    return asio::async_read_until(s, b, DSK_FORWARD(match), use_async_op);
}


} // namespace dsk
