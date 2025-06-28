#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/from_str.hpp>
#include <dsk/charset/ascii.hpp>


namespace dsk{


struct http_range
{
    int64_t first_byte = -1;
    int64_t last_byte = -1; // included
    int64_t suffix_length = -1;

    constexpr bool valid() const noexcept
    {
        return suffix_length > 0
               || (first_byte >= 0
                   && (last_byte < 0 || last_byte >= first_byte));
    }

    constexpr expected<http_range, errc> length_to_range(int64_t len) const
    {
        DSK_ASSERT(valid());

        if(suffix_length > 0)
        {
            return http_range
            {
                .first_byte = len - std::min(len, suffix_length),
                .last_byte  = len - 1
            };
        }

        if(first_byte < len)
        {
            return http_range
            {
                .first_byte = first_byte,
                .last_byte  = (last_byte < 0) ? len - 1
                                              : std::min(len - 1, last_byte)
            };
        }

        return errc::out_of_bound;
    }
};

// https://www.zeng.dev/post/2023-http-range-and-play-mp4-in-browser/
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
// https://source.chromium.org/chromium/chromium/src/+/main:net/http/http_util.cc;l=161;drc=f39c57f31413abcb41d3068cfb2c7a1718003cc5;bpv=0;bpt=1
expected<http_range> parse_single_range_header(_byte_str_ auto const& val)
{
    auto v = str_view<char>(val);
    
    auto p = v.find('=');
    
    if(p == npos)
    {
        return errc::parse_failed;
    }

    if(! ascii_iequal_lower_r(ascii_left_trimed_view(v.substr(0, p)), "bytes"))
    {
        return errc::parse_failed;
    }

    v.remove_prefix(p + 1);

    http_range r;

    p = v.find('-');

    if(p == npos)
    {
        return errc::parse_failed;
    }

    auto t = ascii_left_trimed_view(v.substr(0, p));

    if(! t.empty())
    {
        DSK_E_TRY_ONLY(from_str(t, r.first_byte));
    }

    t = ascii_left_trimed_view(v.substr(p + 1));

    if(! t.empty())
    {
        DSK_E_TRY(auto n, str_to<int64_t>(t));

        if(r.first_byte >= 0) r.last_byte = n;
        else                  r.suffix_length = n;
    }
    else if(r.first_byte < 0)
    {
        return errc::out_of_bound;
    }

    if(! r.valid())
    {
        return errc::out_of_bound;
    }

    return r;
}


} // namespace dsk
