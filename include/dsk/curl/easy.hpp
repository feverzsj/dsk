#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/curl/str.hpp>
#include <dsk/curl/info.hpp>
#include <dsk/curl/err.hpp>


namespace dsk{


inline struct curl_global
{
    curl_global(long f = CURL_GLOBAL_WIN32|CURL_GLOBAL_SSL)
    {
        curl_global_init(f);
    }

    ~curl_global()
    {
        curl_global_cleanup();
    }
} g_curl_global; // user should not define curl_global any more


class curl_easy : public copyable_handle<CURL*, curl_easy_cleanup, curl_easy_duphandle>
{
    using base = copyable_handle<CURL*, curl_easy_cleanup, curl_easy_duphandle>;

public:
    using base::base;

    curl_easy()
        : base(curl_easy_init())
    {
        if(! valid())
        {
            throw std::bad_alloc();
        }
    }

    CURLcode setopt(CURLoption k, auto v)
    {
        return curl_easy_setopt(checked_handle(), k, v);
    }

    auto setopt(auto&& opt)
    {
        return opt(*this);
    }

    auto setopts(auto&&... opts)
    {
        return foreach_elms_until_err<CURLcode>(fwd_as_tuple(opts...), [this](auto& opt)
        {
            return opt(*this);
        });
    }

    void reset_opts() { curl_easy_reset(checked_handle()); }

    CURLcode getinfo(CURLINFO k, auto& v)
    {
        return curl_easy_getinfo(checked_handle(), k, &v);
    }

    template<class Info>
    expected<typename Info::value_type, CURLcode> info(Info const&)
    {
        typename Info::value_type v;
        DSK_E_TRY_ONLY(getinfo(Info::key, v));
        return v;
    }

    template<class T>
    expected<T, CURLcode> priv_data()
    {
        return info(curlinfo::priv_data<T>);
    }

    // Currently the only protocol with a connection upkeep mechanism is HTTP/2:
    // when the connection upkeep interval is exceeded and curl_easy_upkeep is called,
    // an HTTP/2 PING frame is sent on the connection. 
    CURLcode upkeep() noexcept
    {
        return curl_easy_upkeep(checked_handle());
    }

    CURLcode perform() noexcept
    {
        return curl_easy_perform(checked_handle());
    }

    // you must set all state bits in once.
    CURLcode pause(int mask)
    {
        return curl_easy_pause(checked_handle(), mask);
    }

    // the member urlencode/urlencode use registered conv callback if exists,
    // the free curlencode/curlencode use iconv

    // return url encoded s
    curlstr urlencode(_byte_str_ auto const& s)
    {
        return {curl_easy_escape(checked_handle(), str_data<char>(s), str_size<int>(s))};
    }

    curlstr urldecode(_byte_str_ auto const& s)
    {
        return {curl_easy_unescape(checked_handle(), str_data<char>(s), str_size<int>(s))};
    }

    expected<curl_header const&, CURLHcode> header(_byte_str_ auto const& name,
                                                   size_t index = 0,
                                                   unsigned origin = CURLH_HEADER | CURLH_TRAILER,
                                                   int request = -1)
    {
        curl_header* hout = nullptr;

        DSK_E_TRY_ONLY(curl_easy_header(checked_handle(), cstr_data<char>(as_cstr(name)), index, origin, request, &hout));

        return *hout;
    }

    curl_header const* nextheader(curl_header const* prev = nullptr,
                                  unsigned origin = CURLH_HEADER | CURLH_TRAILER,
                                  int request = -1)
    {
        return curl_easy_nextheader(checked_handle(), origin, request, const_cast<curl_header*>(prev));
    }
};


} // namespace dsk


namespace std {

template<> struct hash<dsk::curl_easy>
{
    auto operator()(dsk::curl_easy const& e) const noexcept
    {
        return std::hash<decltype(e.handle())>{}(e.handle());
    }
};

} // namespace std
