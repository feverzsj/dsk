#pragma once

#include <dsk/asio/ip.hpp>
#include <boost/asio/ip/udp.hpp>


namespace dsk{


using udp_protocol = asio::ip::udp;
using udp_endpoint = asio::ip::udp::endpoint;
using udp_resolver = basic_resolver<asio::ip::udp>;

inline auto udp_v4() noexcept { return asio::ip::udp::v4(); }
inline auto udp_v6() noexcept { return asio::ip::udp::v6(); }


class udp_socket :
    public basic_socket<asio::ip::udp>
{
    using base = basic_socket<asio::ip::udp>;

public:
    using base::base;

    auto receive_from(_borrowed_byte_buf_ auto&& mutableBuf, endpoint_type& sender, message_flags f = message_flags(0))
    {
        return base::async_receive_from(asio_buf(DSK_FORWARD(mutableBuf)), sender, f);
    }

    auto receive_from(auto const& mutableBufs, endpoint_type& sender, message_flags f = message_flags(0))
    {
        return base::async_receive_from(mutableBufs, sender, f);
    }

    auto send_to(_borrowed_byte_buf_ auto&& mutableBuf, endpoint_type const& dest, message_flags f = message_flags(0))
    {
        return base::async_send_to(asio_buf(DSK_FORWARD(mutableBuf)), dest, f);
    }

    auto send_to(auto const& mutableBufs, endpoint_type const& dest, message_flags f = message_flags(0))
    {
        return base::async_send_to(mutableBufs, dest, f);
    }
};


} // namespace dsk
