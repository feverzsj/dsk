#pragma once

#include <dsk/asio/tcp.hpp>
#include <dsk/http/msg.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>


namespace dsk{


template<class Socket>
class http_conn_t : public Socket
{
    using base = Socket;

    beast::flat_buffer _buf;

public:
    using base::base;
    using base::read_some;
    using base::read;
    using base::write_some;
    using base::write;

    auto read_some(_http_parser_ auto& p)
    {
        return beast::http::async_read_some(*this, _buf, p, use_async_op);
    }

    // if mp is a message, read whole message.
    // if mp is a parser, read whole or rest of the message.
    // NOTE: p.eager(true) will be called before reading.
    auto read(_http_message_or_parser_ auto& mp)
    {
        return beast::http::async_read(*this, _buf, mp, use_async_op);
    }
    
    // read header only.
    // NOTE: p.eager(false) will be called before reading.
    auto read_header(_http_parser_ auto& p)
    {
        return beast::http::async_read_header(*this, _buf, p, use_async_op);
    }

    // NOTE: p.eager(true) and p.get().body().clear() will be called before reading.
    //       for span_body, you need set the body explicitly and use read_some().
    auto read_some_rest(_http_parser_ auto& p)
    {
        p.eager(true);
        p.get().body().clear();
        return read_some(p);
    }

    // NOTE: p.eager(true) and p.get().body().clear() will be called before reading.
    auto read_rest(_http_parser_ auto& p)
    {
        p.get().body().clear();
        return read(p);
    }

    /// Writing:
    // 
    // Remember to call message.prepare_payload(...) or message.chunked(...),
    // if 'Content-Length' and 'Transfer-Encoding' are not set manually.

    auto write_some(_http_serializer_ auto& s)
    {
        return beast::http::async_write_some(*this, s, use_async_op);
    }

    // if ms is a message, write whole message.
    // if ms is a serializer, write whole or rest of the message.
    // NOTE: s.split(false) will be called before writing.
    auto write(_http_message_or_serializer_ auto&& ms)
    {
        return beast::http::async_write(*this, ms, use_async_op);
    }

    // read header only.
    // NOTE: s.split(true) will be called before writing.
    auto write_header(_http_serializer_ auto& s)
    {
        return beast::http::async_write_header(*this, s, use_async_op);
    }

    template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
    auto write_header(_http_message_ auto&& m)
    {
        return make_hosted_async_op<http_serializer_for<DSK_NO_CVREF_T(m)>, Alloc>
        (
            [this](auto& s){ return beast::http::async_write_header(*this, s, use_async_op); },
            m
        );
    }

    // use write(_byte_buf_) to write all or part of body. The size must match header settings.
    // or use buffer_body:
    // https://www.boost.org/libs/beast/doc/html/beast/more_examples/http_relay.html
    // https://www.boost.org/libs/beast/doc/html/beast/more_examples/send_child_process_output.html

    // Chunked encoding:
    // 
    // https://www.boost.org/libs/beast/doc/html/beast/using_http/chunked_encoding.html


    template<_http_request_or_parser_ Req = http_request>
    task<Req> read_request()
    {
        Req req;
        DSK_TRY read(req);
        DSK_RETURN_MOVED(req);
    }

    task<> read_response(_http_request_or_serializer_ auto& req, _http_response_or_parser_ auto& res)
    {
        DSK_TRY write(req);
        DSK_TRY read(res);
        DSK_RETURN();
    }

    template<_http_response_or_parser_ Res = http_response>
    task<Res> read_response(_http_request_or_serializer_ auto& req)
    {
        Res res;
        DSK_TRY write(req);
        DSK_TRY read(res);
        DSK_RETURN_MOVED(res);
    }
};


using http_conn = http_conn_t<tcp_socket>;


} // namespace dsk
