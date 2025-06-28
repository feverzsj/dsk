#pragma once

#include <dsk/config.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/handle.hpp>
#include <curl/curl.h>
#include <cstring>


namespace dsk{


class curlstr
{
    unique_handle<char*, curl_free> _s;

public:
    explicit curlstr(char* s) : _s(s) {}

    char const* c_str() const noexcept { return _s ? _s.get() : ""; }
    char const* data () const noexcept { return _s.get(); }
    size_t      size () const noexcept { return _s ? std::strlen(_s.get()) : 0; }

    operator std::string_view() const& noexcept { return std::string_view(data(), size()); }
    operator std::string_view() const&& = delete; // forbid binding to all rvalue
};


} // namespace dsk
