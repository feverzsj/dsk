#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/string.hpp>
#include <boost/url/encode.hpp>
#include <boost/url/pct_string_view.hpp>


namespace dsk{


namespace buf_token{


template<_resizable_byte_buf_ B>
struct append_to : boost::urls::string_token::arg
{
    using result_type = B&;

    B& _b;

    explicit append_to(B& b) : _b(b) {}

    char* prepare(size_t n)
    {
        return buy_buf<char>(_b, n);
    }

    result_type result() noexcept
    {
        return _b;
    }
};

template<class B> append_to(B& b) -> append_to<B>;


template<_resizable_byte_buf_ B>
struct assign_to : boost::urls::string_token::arg
{
    using result_type = B&;

    B& _b;

    explicit assign_to(B& b) : _b(b) {}

    char* prepare(size_t n)
    {
        rescale_buf(_b, n);
        return buf_data<char>(_b);
    }

    result_type result() noexcept
    {
        return _b;
    }
};

template<class B> assign_to(B& b) -> assign_to<B>;


template<_resizable_byte_buf_ B>
struct return_t : boost::urls::string_token::arg
{
    using result_type = B;

    B _b;

    char* prepare(size_t n)
    {
        rescale_buf(_b, n);
        return buf_data<char>(_b);
    }

    result_type result() noexcept
    {
        return mut_move(_b);
    }
};

template<_resizable_byte_buf_ B>
inline constexpr return_t<B> return_;


} // namespace buf_token



template<_resizable_byte_buf_ B = string, class Token = buf_token::return_t<B>>
Token::result_type percent_encode(_byte_str_ auto const& s,
                                  auto const& unreserved, // e.g.: boost::urls::unreserved_chars
                                  boost::urls::encoding_opts const& opts = {},
                                  Token&& token = {})
{
    return boost::urls::encode(str_view<char>(s), unreserved, opts, DSK_FORWARD(token));
}


template<_resizable_byte_buf_ B = string, class Token = buf_token::return_t<B>>
expected<typename Token::result_type> percent_decode(_byte_str_ auto const& s,
                                                     boost::urls::encoding_opts const& opts = {},
                                                     Token&& token = {})
{
    DSK_E_TRY_FWD(pv, boost::urls::make_pct_string_view(str_view<char>(s)));
    return pv.decode(opts, DSK_FORWARD(token));
}

template<_resizable_byte_buf_ B>
expected<B&> percent_decode(_byte_str_ auto const& s, B& b,
                            boost::urls::encoding_opts const& opts = {})
{
    return percent_decode(s, opts, buf_token::assign_to(b));
}

template<_resizable_byte_buf_ B>
expected<B&> percent_decode_append(_byte_str_ auto const& s, B& b,
                                   boost::urls::encoding_opts const& opts = {})
{
    return percent_decode(s, opts, buf_token::append_to(b));
}


} // namespace dsk
