#pragma once

#include <dsk/expected.hpp>
#include <dsk/asio/config.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>


namespace dsk{


template<class Protocol>
error_code get_local_endpoint(typename Protocol::socket::native_handle_type s, asio::ip::basic_endpoint<Protocol>& ep) noexcept
{
    error_code ec;
    size_t len = ep.capacity();

    if(asio::detail::socket_ops::getsockname(s, ep.data(), &len, ec) == 0)
    {
        ep.resize(len);
    }

    return ec;
}

template<class Protocol>
error_code get_remote_endpoint(typename Protocol::socket::native_handle_type s, asio::ip::basic_endpoint<Protocol>& ep) noexcept
{
    error_code ec;
    size_t len = ep.capacity();

    if(asio::detail::socket_ops::getpeername(s, ep.data(), &len, false, ec) == 0)
    {
        ep.resize(len);
    }

    return ec;
}


template<class Protocol>
expected<asio::ip::basic_endpoint<Protocol>> get_local_endpoint(typename Protocol::socket::native_handle_type s) noexcept
{
    asio::ip::basic_endpoint<Protocol> ep;
    DSK_U_TRY_ONLY(get_local_endpoint(s, ep));
    return ep;
}

template<class Protocol>
expected<asio::ip::basic_endpoint<Protocol>> get_remote_endpoint(typename Protocol::socket::native_handle_type s) noexcept
{
    asio::ip::basic_endpoint<Protocol> ep;
    DSK_U_TRY_ONLY(get_remote_endpoint(s, ep));
    return ep;
}


} // namespace dsk
