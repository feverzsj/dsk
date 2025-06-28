#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/asio/config.hpp>
#include <dsk/asio/buf.hpp>
#include <dsk/asio/connect.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/basic_resolver_entry.hpp>


namespace dsk{


using ip_addr = asio::ip::address;
using ip_addr_v4 = asio::ip::address_v4;
using ip_addr_v6 = asio::ip::address_v6;


expected<ip_addr> make_ip_addr(_byte_str_ auto const& s)
{
    error_code ec;
    auto addr = asio::ip::make_address(cstr_data<char>(as_cstr(s)), ec);
    return make_expected_if_no(ec, mut_move(addr));
}


template<class Protocol>
class basic_resolver :
    public use_async_op_as_default_with_io_ctx_executor_t<typename Protocol::resolver>
{
    using base = use_async_op_as_default_with_io_ctx_executor_t<typename Protocol::resolver>;

public:
    using base::base;

    basic_resolver()
        : base(DSK_DEFAULT_IO_CONTEXT)
    {}

    auto resolve(auto&&... args)
    requires(requires{
        base::async_resolve(DSK_FORWARD(args)..., use_async_op); })
    {
        return base::async_resolve(DSK_FORWARD(args)..., use_async_op);
    }
};


template<class Protocol>
class basic_socket :
    public use_async_op_as_default_with_io_ctx_executor_t<typename Protocol::socket>
{
    using base = use_async_op_as_default_with_io_ctx_executor_t<typename Protocol::socket>;

public:
    using typename base::wait_type;
    using typename base::shutdown_type;
    using typename base::endpoint_type;
    using typename base::protocol_type;
    using typename base::native_handle_type;
    using message_flags = asio::socket_base::message_flags;

    using base::base;

    basic_socket()
        : base(DSK_DEFAULT_IO_CONTEXT)
    {}

    basic_socket(base&& s)
        : base(mut_move(s))
    {}

    explicit basic_socket(protocol_type const& p)
        : base(DSK_DEFAULT_IO_CONTEXT, p)
    {}

    basic_socket(protocol_type const& p, native_handle_type const& h)
        : base(DSK_DEFAULT_IO_CONTEXT, p, h)
    {}

    explicit basic_socket(endpoint_type const& ep)
        : base(DSK_DEFAULT_IO_CONTEXT, ep)
    {}

    error_code assign(protocol_type const& p, native_handle_type const& h)
    {
        DSK_ASIO_RETURN_EC(base::assign, p, h);
    }

    expected<native_handle_type> release()
    {
        error_code ec;
        auto h = base::release(ec);
        return make_expected_if_no(ec, h);
    }

    error_code bind(endpoint_type const& ep)
    {
        DSK_ASIO_RETURN_EC(base::bind, ep);
    }

    error_code get_option(auto& opt) const
    {
        DSK_ASIO_RETURN_EC(base::get_option, opt);
    }

    error_code set_option(auto const& opt)
    {
        DSK_ASIO_RETURN_EC(base::set_option, opt);
    }

    error_code io_control(auto& cmd)
    {
        DSK_ASIO_RETURN_EC(base::io_control, cmd);
    }

    error_code open(protocol_type const& p)
    {
        DSK_ASIO_RETURN_EC(base::open, p);
    }

    error_code close()
    {
        DSK_ASIO_RETURN_EC(base::close);
    }

    error_code cancel()
    {
        DSK_ASIO_RETURN_EC(base::cancel);
    }

    bool non_blocking() const
    {
        return base::non_blocking();
    }

    error_code non_blocking(bool on)
    {
        DSK_ASIO_RETURN_EC(base::non_blocking, on);
    }

    bool native_non_blocking() const
    {
        return base::native_non_blocking();
    }

    error_code native_non_blocking(bool on)
    {
        DSK_ASIO_RETURN_EC(base::native_non_blocking, on);
    }

    error_code shutdown(shutdown_type what)
    {
        DSK_ASIO_RETURN_EC(base::shutdown, what);
    }

    expected<bool> at_mark() const
    {
        DSK_ASIO_RETURN_EXPECTED(base::at_mark);
    }

    expected<size_t> available() const
    {
        DSK_ASIO_RETURN_EXPECTED(base::available);
    }

    expected<endpoint_type> local_endpoint() const
    {
        DSK_ASIO_RETURN_EXPECTED(base::local_endpoint);
    }

    expected<endpoint_type> remote_endpoint() const
    {
        DSK_ASIO_RETURN_EXPECTED(base::remote_endpoint);
    }

    auto connect(endpoint_type const& endpoint)
    {
        return base::async_connect(endpoint);
    }

    auto connect(_range_of_<asio::ip::basic_resolver_entry<Protocol>> auto&& endpoints, auto&&... connCond)
    {
        return dsk::connect(*this, DSK_FORWARD(endpoints), DSK_FORWARD(connCond)...);
    }

    auto connect(ip_addr const& addr, asio::ip::port_type port)
    {
        return connect(endpoint_type(addr, port));
    }

    auto wait(wait_type w)
    {
        return base::async_wait(w);
    }

    auto receive(_borrowed_byte_buf_ auto&& mutableBuf, message_flags f = message_flags(0))
    {
        return base::async_receive(asio_buf(DSK_FORWARD(mutableBuf)), f);
    }

    auto receive(auto const& mutableBufs, message_flags f = message_flags(0))
    {
        return base::async_receive(mutableBufs, f);
    }

    auto send(_borrowed_byte_buf_ auto&& constBuf, message_flags f = message_flags(0))
    {
        return base::async_send(asio_buf(DSK_FORWARD(constBuf)), f);
    }

    auto send(auto const& constBufs, message_flags f = message_flags(0))
    {
        return base::async_send(constBufs, f);
    }
};


} // namespace dsk
