#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/asio/buf.hpp>
#include <dsk/http/msg.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <string_view>


namespace dsk{


// only insert name value into fields, while beast.http.parser will also set other members in header
error_code parse_fields(_byte_str_ auto const& in, _http_fields_ auto& fields)
{
    error_code ec;
    beast::string_view name, value;
    beast::detail::char_buffer<beast::http::detail::basic_parser_base::max_obs_fold> buf;

    char const* p = str_begin<char>(in);
    char const* const last = str_end<char>(in);

    for(;;)
    {
        if(p + 2 > last) return beast::http::error::need_more;

        if(p[0] == '\r')
        {
            if(p[1] == '\n') return {};
            else             return beast::http::error::bad_line_ending;
        }

        beast::http::detail::basic_parser_base::parse_field(p, last, name, value, buf, ec);

        if(ec) return ec;

        fields.insert(name, value);
    }

    return {};
}


// Assumes b contains one or more headers and an optional trailer at the end:
//      HTTP/...\r\n
//      [header fields] \r\n
//      \r\n
//      ...
//      [trailer header fields] \r\n
//      \r\n
// which matches CURLOPT_HEADERFUNCTION behavior.
error_code parse_header_trailer(_byte_str_ auto const& b, _http_header_ auto& h)
{
    auto&& bv = str_view<char>(b);

    if(bv.size() < 16) return beast::http::error::need_more;

    std::string_view header = bv;
    std::string_view trailer;

    if(auto q = bv.rfind("\r\n\r\n", bv.size() - 5); q != npos)
    {
        if(auto t = bv.substr(q + 4); t.starts_with("HTTP/"))
        {
            header = t;
        }
        else
        {
            trailer = t;

            if(auto j = bv.rfind("\r\n\r\n", q - 1); j != npos) header = bv.substr(j + 4, q - j);
            else                                                header = bv.substr(0, q + 4);
        }
    }

    beast::http::response_parser<
        beast::http::empty_body,
        typename DSK_NO_CVREF_T(h)::allocator_type> parser;

    parser.skip(true); // complete when the header is done

    error_code ec;

    parser.put(asio_buf(header), ec);

    if(ec) return ec;

    DSK_ASSERT(parser.is_header_done());

    if(trailer.size())
    {
        ec = parse_fields(trailer, parser.get());
        if(ec) return ec;
    }

    h = parser.release();
    return {};
}


error_code fill_response_body(_byte_str_ auto const& b, _http_response_ auto& m)
{
    auto&& bv = str_view<char>(b);

    typename DSK_NO_REF_T(m)::body_type::reader r(m.base(), m.body());

    error_code ec;

    r.init(ec, bv.size());
    if(ec) return ec;

    r.put(asio_buf(bv), ec);
    if(ec) return ec;

    r.finish(ec);
    if(ec) return ec;

    return {};
}


} // namespace dsk
