#pragma once

#include <dsk/util/str.hpp>
#include <dsk/asio/tcp.hpp>
#include <dsk/asio/buf.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <boost/asio/ssl.hpp>


namespace dsk{


constexpr decltype(auto) as_std_string(_byte_str_ auto&& s)
{
    if constexpr(_no_cvref_same_as_<decltype(s), std::string>)
    {
        return DSK_FORWARD(s);
    }
    else
    {
        return std::string(str_view<char>(s));
    }
}


auto ssl_host_name_verification(_byte_str_ auto const& host)
{
    return asio::ssl::host_name_verification(as_std_string(host));
}


using ssl_handshake = asio::ssl::stream_base;
using ssl_verify_context = asio::ssl::verify_context;


class ssl_context : public asio::ssl::context
{
    using base = asio::ssl::context;

public:
    using base::base;

    error_code clear_options(options o)
    {
        DSK_ASIO_RETURN_EC(base::clear_options, o);
    }

    error_code set_options(options o)
    {
        DSK_ASIO_RETURN_EC(base::set_options, o);
    }

    error_code set_verify_mode(asio::ssl::verify_mode m)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_mode, m);
    }

    error_code set_verify_depth(int depth)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_depth, depth);
    }

    error_code set_verify_callback(auto&& f)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_callback, DSK_FORWARD(f));
    }

    error_code set_password_callback(auto&& f)
    {
        DSK_ASIO_RETURN_EC(base::set_password_callback, DSK_FORWARD(f));
    }

    error_code load_verify_file(_byte_str_ auto const& filename)
    {
        DSK_ASIO_RETURN_EC(base::load_verify_file, as_std_string(filename));
    }

    error_code add_certificate_authority(_borrowed_byte_buf_ auto&& c)
    {
        DSK_ASIO_RETURN_EC(base::add_certificate_authority, asio_buf(DSK_FORWARD(c)));
    }

    error_code set_default_verify_paths()
    {
        DSK_ASIO_RETURN_EC(base::set_default_verify_paths);
    }

    error_code add_verify_path(_byte_str_ auto const& path)
    {
        DSK_ASIO_RETURN_EC(base::add_verify_path, as_std_string(path));
    }

    error_code use_certificate(_borrowed_byte_buf_ auto&& c, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_certificate, asio_buf(DSK_FORWARD(c)), f);
    }

    error_code use_certificate_file(_byte_str_ auto const& filename, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_certificate_file, as_std_string(filename), f);
    }

    error_code use_private_key(_borrowed_byte_buf_ auto&& k, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_private_key, asio_buf(DSK_FORWARD(k)), f);
    }

    error_code use_private_key_file(_byte_str_ auto const& filename, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_private_key_file, as_std_string(filename), f);
    }

    error_code use_rsa_private_key(_borrowed_byte_buf_ auto&& k, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_rsa_private_key, asio_buf(DSK_FORWARD(k)), f);
    }

    error_code use_rsa_private_key_file(_byte_str_ auto const& filename, file_format f)
    {
        DSK_ASIO_RETURN_EC(base::use_rsa_private_key_file, as_std_string(filename), f);
    }

    error_code use_certificate_chain(_borrowed_byte_buf_ auto&& c)
    {
        DSK_ASIO_RETURN_EC(base::use_certificate_chain, asio_buf(DSK_FORWARD(c)));
    }

    error_code use_certificate_chain_file(_byte_str_ auto const& filename)
    {
        DSK_ASIO_RETURN_EC(base::use_certificate_chain_file, as_std_string(filename));
    }

    error_code use_tmp_dh(_borrowed_byte_buf_ auto&& d)
    {
        DSK_ASIO_RETURN_EC(base::use_tmp_dh, asio_buf(DSK_FORWARD(d)));
    }

    error_code use_tmp_dh_file(_byte_str_ auto const& filename)
    {
        DSK_ASIO_RETURN_EC(base::use_tmp_dh_file, as_std_string(filename));
    }
};


template<class Stream>
class ssl_stream : public asio::ssl::stream<Stream>
{
    using base = asio::ssl::stream<Stream>;

public:
    using typename base::handshake_type;

    using base::base;

    ssl_stream(base&& b)
        : base(mut_move(b))
    {}

    explicit ssl_stream(asio::ssl::context& ctx) requires(! _ref_<Stream>)
        : base(DSK_DEFAULT_IO_CONTEXT, ctx)
    {}

    error_code set_verify_mode(asio::ssl::verify_mode m)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_mode, m);
    }

    error_code set_verify_depth(int depth)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_depth, depth);
    }

    error_code set_verify_callback(auto&& f)
    {
        DSK_ASIO_RETURN_EC(base::set_verify_callback, DSK_FORWARD(f));
    }

    auto handshake(handshake_type t)
    {
        return base::async_handshake(t, use_async_op);
    }

    auto handshake(handshake_type t, _borrowed_byte_buf_ auto&& b)
    {
        return base::async_handshake(t, asio_buf(DSK_FORWARD(b)), use_async_op);
    }

    auto handshake(handshake_type t, auto const& bs)
    {
        return base::async_handshake(t, bs, use_async_op);
    }

    auto shutdown()
    {
        return base::async_shutdown(use_async_op);
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


} // namespace dsk
