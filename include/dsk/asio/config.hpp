#pragma once

#include <dsk/expected.hpp>


namespace boost::asio{}


namespace dsk{


namespace asio = boost::asio;

#define DSK_ASIO_RETURN_EC(fun, ...)   \
    error_code ec;                     \
    fun(__VA_ARGS__ __VA_OPT__(,) ec); \
    return ec;                         \
/**/

#define DSK_ASIO_RETURN_EXPECTED(fun, ...)            \
    error_code ec;                                    \
    auto&& dre = fun(__VA_ARGS__ __VA_OPT__(,) ec);   \
    return make_expected_if_no(ec, DSK_FORWARD(dre)); \
/**/


error_code asio_invoke_and_return_ec(auto* p, auto m, auto&&... args)
{
    error_code ec;
    p->*m(DSK_FORWARD(args)..., ec);
    return ec;
}

auto asio_invoke_and_return_expected(auto* p, auto m, auto&&... args)
{
    error_code ec;
    auto&& r = p->*m(DSK_FORWARD(args)..., ec);
    return make_expected_if_no(ec, DSK_FORWARD(r));
}


} // namespace dsk
