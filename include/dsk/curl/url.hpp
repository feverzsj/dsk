#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/curl/str.hpp>
#include <dsk/curl/err.hpp>
#include <map>


namespace dsk{


template<class T>
using curlu_expected = expected<T, CURLUcode>;


class curlu : public copyable_handle<CURLU*, curl_url_cleanup, curl_url_dup>
{
    using base = copyable_handle<CURLU*, curl_url_cleanup, curl_url_dup>;

public:
    // flags:
    enum : unsigned
    {                                                     // [used by]
        show_port             = CURLU_DEFAULT_PORT,       // [get] if port isn't set explicitly, show the default port.
        hide_default_port     = CURLU_NO_DEFAULT_PORT,    // [get] if port is set and is same as default, don't show it.
        allow_default_scheme  = CURLU_DEFAULT_SCHEME,     // [get/set] use default scheme if missing.
        allow_empty_authority = CURLU_NO_AUTHORITY,       // [set] allow empty authority(HostName + Port) when the scheme is unknown.
        allow_unknown_scheme  = CURLU_NON_SUPPORT_SCHEME, // [set] allow non-supported scheme.
        no_usr_pw             = CURLU_DISALLOW_USER,      // [set] no user+password allowed.
        path_as_is            = CURLU_PATH_AS_IS,         // [set] leave dot sequences on set.
        allow_space           = CURLU_ALLOW_SPACE,        // [set] allows space (ASCII 32) where possible.
        decode_on_get         = CURLU_URLDECODE,          // [get] URL decode on get.
        encode_on_set         = CURLU_URLENCODE,          // [set] URL encode on set.
        guess_scheme          = CURLU_GUESS_SCHEME,       // [set] legacy curl-style guessing.
        punycode              = CURLU_PUNYCODE,           // [get] get host/url in punycode, if NOT encode_on_set 
        puny2idn              = CURLU_PUNY2IDN,           // [get] like punycode, but get host name in IDN UTF-8 if it is punycode.
        //get_empty             = CURLU_GET_EMPTY,          // [get] treat empty query and fragments parts as existing.
    };

    using base::base;

    curlu()
        : base(curl_url())
    {
        if(! valid())
        {
            throw std::bad_alloc();
        }
    }

    explicit curlu(_byte_str_ auto const& s, unsigned flags = encode_on_set)
        : curlu()
    {
        set(CURLUPART_URL, s, flags);
    }

    CURLUcode try_set(CURLUPart what, _byte_str_ auto const& part, unsigned flags = encode_on_set)
    {
        return curl_url_set(checked_handle(), what, cstr_data<char>(as_cstr(part)), flags);
    }

    curlu& set(CURLUPart what, _byte_str_ auto const& part, unsigned flags = encode_on_set)
    {
        throw_on_err(try_set(what, part, flags));
        return *this;
    }

    curlu_expected<curlstr> get(CURLUPart what, unsigned flags = 0) const
    {
        char* part = nullptr;

        if(auto e = curl_url_get(checked_handle(), what, &part, flags))
        {
            return e;
        }

        return curlstr(part);
    }

    curlu_expected<curlstr> get_decoded(CURLUPart what, unsigned flags = 0) const
    {
        return get(what, flags | decode_on_get);
    }

    curlu_expected<curlstr> url     (unsigned flags = 0) const { return get(CURLUPART_URL     , flags); }
    curlu_expected<curlstr> scheme  (unsigned flags = 0) const { return get(CURLUPART_SCHEME  , flags); }
    curlu_expected<curlstr> user    (unsigned flags = 0) const { return get(CURLUPART_USER    , flags); }
    curlu_expected<curlstr> password(unsigned flags = 0) const { return get(CURLUPART_PASSWORD, flags); }
    curlu_expected<curlstr> options (unsigned flags = 0) const { return get(CURLUPART_OPTIONS , flags); }
    curlu_expected<curlstr> host    (unsigned flags = 0) const { return get(CURLUPART_HOST    , flags); }
    curlu_expected<curlstr> port    (unsigned flags = 0) const { return get(CURLUPART_PORT    , flags); }
    curlu_expected<curlstr> path    (unsigned flags = 0) const { return get(CURLUPART_PATH    , flags); }
    curlu_expected<curlstr> query   (unsigned flags = 0) const { return get(CURLUPART_QUERY   , flags); }
    curlu_expected<curlstr> fragment(unsigned flags = 0) const { return get(CURLUPART_FRAGMENT, flags); }
    curlu_expected<curlstr> zoneid  (unsigned flags = 0) const { return get(CURLUPART_ZONEID  , flags); } // ipv6 zone id

    curlu_expected<curlstr> decoded_url  (unsigned flags = 0) const { return url  (flags | decode_on_get); }
    curlu_expected<curlstr> decoded_query(unsigned flags = 0) const { return query(flags | decode_on_get); }

    // set new url;
    // if s is a relative url and a url was already set, redirect the url to s
    curlu& url     (_byte_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_URL     , s, flags); }
    curlu& scheme  (_byte_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_SCHEME  , s, flags); }
    curlu& user    (_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_USER    , s, flags); }
    curlu& password(_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_PASSWORD, s, flags); }
    curlu& options (_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_OPTIONS , s, flags); }
    curlu& host    (_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_HOST    , s, flags); }
    curlu& port    (_byte_str_ auto const& s, unsigned flags = 0            ) & { return set(CURLUPART_PORT    , s, flags); }
    curlu& port    (        unsigned short n, unsigned flags = 0            ) & { return port(stringify(n)     ,    flags); }
    curlu& path    (_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_PATH    , s, flags); } // all '/' are preserved, other characters are percent encoded when encode_on_set.
    curlu& fragment(_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_FRAGMENT, s, flags); }
    curlu& zoneid  (_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_ZONEID  , s, flags); } // ipv6 zone id

    curlu&& url     (_byte_str_ auto const& s, unsigned flags = 0            ) && { return mut_move(url     (s, flags)); }
    curlu&& scheme  (_byte_str_ auto const& s, unsigned flags = 0            ) && { return mut_move(scheme  (s, flags)); }
    curlu&& user    (_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(user    (s, flags)); }
    curlu&& password(_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(password(s, flags)); }
    curlu&& options (_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(options (s, flags)); }
    curlu&& host    (_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(host    (s, flags)); }
    curlu&& port    (_byte_str_ auto const& s, unsigned flags = 0            ) && { return mut_move(port    (s, flags)); }
    curlu&& port    (        unsigned short n, unsigned flags = 0            ) && { return mut_move(port    (n, flags)); }
    curlu&& path    (_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(path    (s, flags)); }
    curlu&& fragment(_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(fragment(s, flags)); }
    curlu&& zoneid  (_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(zoneid  (s, flags)); }

    // if flags & encode_on_set, 's' must has the form of "name=value", since only the first '=' is preserved, other characters are percent encoded
    curlu& set_query_str(_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set(CURLUPART_QUERY, s, flags); }
    curlu& append_query_str(_byte_str_ auto const& s, unsigned flags = encode_on_set) & { return set_query_str(s, flags | CURLU_APPENDQUERY); }
    curlu& query_str(_byte_str_ auto const&... s) & { return (... , append_query_str(s)); }
    curlu& query(_byte_str_ auto const& k, _stringifible_ auto const& v) & { return append_query_str(cat_as_str(k, "=", v)); }
    curlu& queries(auto const&... kvs) & { return (... , query(kvs)); }

    curlu&& set_query_str(_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(set_query_str(s, flags)); }
    curlu&& append_query_str(_byte_str_ auto const& s, unsigned flags = encode_on_set) && { return mut_move(append_query_str(s, flags)); }
    curlu&& query_str(_byte_str_ auto const&... s) && { return mut_move(query_str(s...)); }
    curlu&& queries(auto const&... kvs) && { return mut_move(query(kvs...)); }
};


curlu_expected<curlstr> encode_url(_byte_str_ auto const& u){ return curlu{u}.url(); }
curlu_expected<curlstr> decode_url(_byte_str_ auto const& u){ return curlu{u, 0}.decoded_url(); }


// URL escape/unescape the whole string, so don't pass in whole url

curlstr curl_escape(_byte_str_ auto const& s)
{
    return curlstr(curl_easy_escape(nullptr, str_data<char>(s), str_size<int>(s)));
}

curlstr curl_unescape(_byte_str_ auto const& s)
{
    return curlstr(curl_easy_unescape(nullptr, str_data<char>(s), str_size<int>(s)));
}


} // namespace dsk
