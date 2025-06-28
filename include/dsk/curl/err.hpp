#pragma once

#include <dsk/err.hpp>
#include <curl/curl.h>


namespace dsk{


// CURLcode

class curl_err_category : public error_category
{
public:
	char const* name() const noexcept override { return "curl_easy"; }
	std::string message(int rc) const override { return curl_easy_strerror(static_cast<CURLcode>(rc)); }
};

inline constexpr curl_err_category g_curl_err_cat;


// CURLMcode

class curlm_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "curl_multi"; }
    std::string message(int rc) const override { return curl_multi_strerror(static_cast<CURLMcode>(rc)); }
};

inline constexpr curlm_err_category g_curlm_err_cat;


// CURLSHcode

class curlsh_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "curl_share"; }
    std::string message(int rc) const override { return curl_share_strerror(static_cast<CURLSHcode>(rc)); }
};

inline constexpr curlsh_err_category g_curlsh_err_cat;


// CURLUcode

class curlu_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "curl_url"; }

    std::string message(int rc) const override { return curl_url_strerror(static_cast<CURLUcode>(rc)); }
};

inline constexpr curlu_err_category g_curlu_err_cat;


// CURLHcode

class curlh_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "curl_header"; }

    std::string message(int rc) const override
    {
        switch(static_cast<CURLHcode>(rc))
        {
            case CURLHE_BADINDEX     : return "header exists but not with this index";
            case CURLHE_MISSING      : return "no such header exists";
            case CURLHE_NOHEADERS    : return "no headers at all exist (yet)";
            case CURLHE_NOREQUEST    : return "no request with this number was used";
            case CURLHE_OUT_OF_MEMORY: return "out of memory while processing";
            case CURLHE_BAD_ARGUMENT : return "a function argument was not okay";
            case CURLHE_NOT_BUILT_IN : return "if API was disabled in the build";
            default                  : return "undefined";
        }
    }
};

inline constexpr curlh_err_category g_curlh_err_cat;

} // namespace dsk


DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(CURLcode  , dsk::  g_curl_err_cat)
DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(CURLMcode , dsk:: g_curlm_err_cat)
DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(CURLSHcode, dsk::g_curlsh_err_cat)
DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(CURLUcode , dsk:: g_curlu_err_cat)
DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(CURLHcode , dsk:: g_curlh_err_cat)
