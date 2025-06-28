#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <simdutf.h>


namespace dsk{


enum class base64_errc
{
    buffer_mismatch = 1
};


class base64_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "base64"; }

    std::string message(int condition) const override
    {
        switch(static_cast<base64_errc>(condition))
        {
            case base64_errc::buffer_mismatch : return "Buffer mismatch";
        }

        return "undefined";
    }
};

inline constexpr base64_err_category g_base64_err_cat;


class simdutf_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "simdutf"; }

    std::string message(int condition) const override
    {
        switch(static_cast<simdutf::error_code>(condition))
        {
            case simdutf::HEADER_BITS              : return "Any byte must have fewer than 5 header bits";
            case simdutf::TOO_SHORT                : return "Encoded sequence too short";
            case simdutf::TOO_LONG                 : return "Encoded sequence too long";
            case simdutf::OVERLONG                 : return "Decoded character overlong";
            case simdutf::TOO_LARGE                : return "Decoded character too large";
            case simdutf::SURROGATE                : return "Decoded character is surrogate";
            case simdutf::INVALID_BASE64_CHARACTER : return "Invalid base64 character";
            case simdutf::BASE64_INPUT_REMAINDER   : return "Missing padding";
            case simdutf::OUTPUT_BUFFER_TOO_SMALL  : return "output buffer too small";
            case simdutf::OTHER                    : return "Other error";
            default: break; // for unhandled enums
        }

        return "undefined";
    }
};

inline constexpr simdutf_err_category g_simdutf_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, base64_errc, g_base64_err_cat)
DSK_REGISTER_ERROR_CODE_ENUM(simdutf, error_code, dsk::g_simdutf_err_cat)


namespace dsk{


void append_as_base64(_resizable_byte_buf_ auto& d, auto const& v, simdutf::base64_options opts = simdutf::base64_default)
{
    auto&& s = as_buf_of<char>(v);

    simdutf::binary_to_base64(
        buf_data(s),
        buf_size(s),
        buy_buf<char>(d, simdutf::base64_length_from_binary(buf_size(s), opts)),
        opts);
}

template<_resizable_byte_buf_ T = string>
T as_base64(auto const& v, simdutf::base64_options opts = simdutf::base64_default)
{
    T d;
    append_as_base64(d, v, opts);
    return d;
}


simdutf::error_code append_from_base64(_resizable_byte_buf_ auto& d, _byte_str_ auto const& s,
                                       simdutf::base64_options opts = simdutf::base64_default)
{
    auto* sd = reinterpret_cast<char const*>(str_data(s));
    auto  ss = str_size(s);
    auto  ds = buf_size(d);
    auto* dd = buy_buf(d, simdutf::maximal_binary_length_from_base64(sd, ss));

    auto[e, n] = simdutf::base64_to_binary(sd, ss, reinterpret_cast<char*>(dd), opts);

    if(! e)
    {
        resize_buf(d, ds + n);
        return {};
    }

    return e;
}

error_code from_base64(_byte_str_ auto const& s, auto& v, simdutf::base64_options opts = simdutf::base64_default)
{
    if constexpr(_resizable_byte_buf_<decltype(v)>)
    {
        clear_buf(v);
        return append_from_base64(v, s, opts);
    }
    else
    {
        auto&& b  = as_buf_of<char>(v);
        size_t bs = buf_size(b);

        auto[e, n] = simdutf::base64_to_binary_safe(str_data(s),
                                                    str_size(s),
                                                    reinterpret_cast<char*>(buf_data(b)),
                                                    bs,
                                                    opts);

        if(! e)
        {
            DSK_ASSERT(n == str_size(s));

            if(bs == buf_size(b)) return {};
            else                  return base64_errc::buffer_mismatch;
        }

        if(e == simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL) return base64_errc::buffer_mismatch;
        else                                                  return e;
    }
}

template<class T>
expected<T> from_base64(_byte_str_ auto const& s, simdutf::base64_options opts = simdutf::base64_default)
{
    T v;
    DSK_E_TRY_ONLY(from_base64(s, v, opts));
    return v;
}


} // namespace dsk
