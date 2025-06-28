#pragma once

#include <dsk/default_allocator.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/span_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>


namespace dsk{


namespace beast = boost::beast;


using http_verb = beast::http::verb;
using http_field = beast::http::field;
using http_status = beast::http::status;

using http_fields          = beast::http::basic_fields<DSK_DEFAULT_ALLOCATOR<char>>;
using http_request_header  = beast::http::request_header<http_fields>;
using http_response_header = beast::http::response_header<http_fields>;

using http_span_body = beast::http::span_body<char>;
using http_empty_body = beast::http::empty_body;
using http_string_body = beast::http::basic_string_body<char, std::char_traits<char>,
                                                        DSK_DEFAULT_ALLOCATOR<char>>;


template<class Body, class Fields = http_fields>
using http_request_t  = beast::http::request<Body, Fields>;

template<class Body, class Fields = http_fields>
using http_response_t = beast::http::response<Body, Fields>;

using http_request  = http_request_t<http_string_body>;
using http_response = http_response_t<http_string_body>;


template<class Body, class Alloc = DSK_DEFAULT_ALLOCATOR<char>>
using http_request_parser_t  = beast::http::request_parser<Body, Alloc>;

template<class Body, class Alloc = DSK_DEFAULT_ALLOCATOR<char>>
using http_response_parser_t  = beast::http::response_parser<Body, Alloc>;

using http_request_parser  = http_request_parser_t<http_string_body>;
using http_response_parser = http_response_parser_t<http_string_body>;


template<class Body, class Fields = http_fields>
using http_request_serializer_t  = beast::http::request_serializer<Body, Fields>;

template<class Body, class Fields = http_fields>
using http_response_serializer_t  = beast::http::response_serializer<Body, Fields>;

using http_request_serializer  = http_request_serializer_t<http_string_body>;
using http_response_serializer = http_response_serializer_t<http_string_body>;


template<class M>
using http_serializer_for  = beast::http::serializer<M::is_request::value, typename M::body_type, typename M::fields_type>;


template<class T> concept _http_fields_ = _specialized_from_<T, beast::http::basic_fields>;

template<bool IsRequest, class Fields> void _http_header_impl(beast::http::header<IsRequest, Fields>&);
template<class T> concept _http_header_ = requires(std::remove_cvref_t<T>& t){ dsk::_http_header_impl(t); };

template<bool IsRequest, class Body, class Fields> void _http_message_impl(beast::http::message<IsRequest, Body, Fields>&);
template<class T> concept _http_message_  = requires(std::remove_cvref_t<T>& t){ dsk::_http_message_impl(t); };
template<class T> concept _http_request_  = requires(std::remove_cvref_t<T>& t){ dsk::_http_message_impl<true>(t); };
template<class T> concept _http_response_ = requires(std::remove_cvref_t<T>& t){ dsk::_http_message_impl<false>(t); };

template<bool IsRequest> void _http_parser_impl(beast::http::basic_parser<IsRequest>&);
template<class T> concept _http_parser_          = requires(std::remove_cvref_t<T>& t){ dsk::_http_parser_impl(t); };
template<class T> concept _http_request_parser_  = requires(std::remove_cvref_t<T>& t){ dsk::_http_parser_impl<true>(t); };
template<class T> concept _http_response_parser_ = requires(std::remove_cvref_t<T>& t){ dsk::_http_parser_impl<false>(t); };

template<bool IsRequest, class Body, class Fields> void _http_serializer_impl(beast::http::serializer<IsRequest, Body, Fields>&);
template<class T> concept _http_serializer_          = requires(std::remove_cvref_t<T>& t){ dsk::_http_serializer_impl(t); };
template<class T> concept _http_request_serializer_  = requires(std::remove_cvref_t<T>& t){ dsk::_http_serializer_impl<true>(t); };
template<class T> concept _http_response_serializer_ = requires(std::remove_cvref_t<T>& t){ dsk::_http_serializer_impl<false>(t); };


template<class T> concept _http_message_or_parser_  = _http_message_<T> || _http_parser_<T>;
template<class T> concept _http_request_or_parser_  = _http_request_<T> || _http_request_parser_<T>;
template<class T> concept _http_response_or_parser_ = _http_response_<T> || _http_response_parser_<T>;

template<class T> concept _http_message_or_serializer_  = _http_message_<T> || _http_serializer_<T>;
template<class T> concept _http_request_or_serializer_  = _http_request_<T> || _http_request_serializer_<T>;
template<class T> concept _http_response_or_serializer_ = _http_response_<T> || _http_response_serializer_<T>;


} // namespace dsk
