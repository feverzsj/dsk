#pragma once

#include <dsk/asio/ip.hpp>
#include <dsk/asio/read.hpp>
#include <dsk/asio/read_until.hpp>
#include <dsk/asio/write.hpp>
#include <dsk/asio/endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace dsk{


using tcp_protocol = asio::ip::tcp;
using tcp_endpoint = asio::ip::tcp::endpoint;
using tcp_resolver = basic_resolver<asio::ip::tcp>;

inline auto tcp_v4() noexcept { return asio::ip::tcp::v4(); }
inline auto tcp_v6() noexcept { return asio::ip::tcp::v6(); }


class tcp_socket :
    public basic_socket<asio::ip::tcp>
{
    using base = basic_socket<asio::ip::tcp>;

public:
    using base::base;
    using base::assign;

    error_code assign(native_handle_type s) noexcept
    {
        DSK_E_TRY_FWD(ep, get_local_endpoint<asio::ip::tcp>(s));
        return assign(ep.protocol(), s);
    }

    auto read_some(_borrowed_byte_buf_ auto&& mutableBuf)
    {
        return base::async_read_some(asio_buf(DSK_FORWARD(mutableBuf)));
    }

    auto read_some(auto const& mutableBufs)
    {
        return base::async_read_some(mutableBufs);
    }

    auto write_some(_borrowed_byte_buf_ auto&& constBuf)
    {
        return base::async_write_some(asio_buf(DSK_FORWARD(constBuf)));
    }

    auto write_some(auto const& constBufs)
    {
        return base::async_write_some(constBufs);
    }

    auto read(auto&& b, auto&&... compCond)
    {
        return dsk::read(*this, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }

    auto write(auto&& b, auto&&... compCond)
    {
        return dsk::write(*this, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }

    auto read_until(auto& b, auto&& match)
    {
        return dsk::read_until(*this, b, DSK_FORWARD(match));
    }
};


// https://stackoverflow.com/questions/31125229/accept-ipv4-and-ipv6-together-in-boostasio
class tcp_acceptor :
    public use_async_op_as_default_with_io_ctx_executor_t<asio::ip::tcp::acceptor>
{
    using base = use_async_op_as_default_with_io_ctx_executor_t<asio::ip::tcp::acceptor>;

public:
    using base::base;

    tcp_acceptor()
        : base(DSK_DEFAULT_IO_CONTEXT)
    {}

    explicit tcp_acceptor(protocol_type const& p)
        : base(DSK_DEFAULT_IO_CONTEXT, p)
    {}

    tcp_acceptor(protocol_type const& p, native_handle_type const& h)
        : base(DSK_DEFAULT_IO_CONTEXT, p, h)
    {}

    explicit tcp_acceptor(endpoint_type const& ep, bool reuseAddr = true)
        : base(DSK_DEFAULT_IO_CONTEXT, ep, reuseAddr)
    {}

    error_code assign(protocol_type const& p, native_handle_type const& h)
    {
        DSK_ASIO_RETURN_EC(base::assign, p, h);
    }

    expected<native_handle_type> release()
    {
        DSK_ASIO_RETURN_EXPECTED(base::release);
    }

    error_code open(protocol_type const& p)
    {
        DSK_ASIO_RETURN_EC(base::open, p);
    }

    error_code get_option(auto& opt) const
    {
        DSK_ASIO_RETURN_EC(base::get_option, opt);
    }

    error_code set_option(auto const& opt)
    {
        DSK_ASIO_RETURN_EC(base::set_option, opt);
    }

    error_code bind(endpoint_type const& ep)
    {
        DSK_ASIO_RETURN_EC(base::bind, ep);
    }

    error_code bind(protocol_type const& p, asio::ip::port_type port)
    {
        return bind(endpoint_type(p, port));
    }

    error_code bind(ip_addr const& addr, asio::ip::port_type port)
    {
        return bind(endpoint_type(addr, port));
    }

    error_code io_control(auto& cmd)
    {
        DSK_ASIO_RETURN_EC(base::io_control, cmd);
    }

    error_code listen(int backlog = socket_base::max_listen_connections)
    {
        DSK_ASIO_RETURN_EC(base::listen, backlog);
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

    auto accept(auto&&... args)
    requires(requires{
        base::async_accept(DSK_FORWARD(args)..., use_async_op); })
    {
        return base::async_accept(DSK_FORWARD(args)..., use_async_op);
    }

    template<class Socket = tcp_socket>
    auto accept()
    {
        return base::async_accept(use_async_op_for<Socket>);
    }

    auto wait(wait_type w)
    {
        return base::async_wait(w);
    }
};


} // namespace dsk
