#pragma once

#include <dsk/config.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/unit.hpp>
#include <dsk/curl/url.hpp>
#include <dsk/curl/str.hpp>
#include <dsk/curl/list.hpp>
#include <dsk/curl/share.hpp>
#include <curl/curl.h>
#include <chrono>


namespace dsk::curlopt{


template<auto Opt>
inline constexpr auto long_opt = [](long v)
{
    return [v](auto& h){ return h.setopt(Opt, v); };
};

template<auto Opt>
inline constexpr auto offt_opt = [](curl_off_t v)
{
    return [v](auto& h){ return h.setopt(Opt, v); };
};


// template<auto Opt>
// inline constexpr auto bool_opt = [](bool v = true){ return [v](auto& h){ return h.setopt(Opt, v ? 1L : 0L); }; };
template<auto Opt>                  // ^^^^^^^^^^^^^: to write something like setopt(some_bool_opt), we need struct below
struct bool_opt_t
{
    bool v = true;

    constexpr bool_opt_t operator()(bool t) const
    {
        return {t};
    }

    constexpr auto operator()(auto& h) const
    {
        return h.setopt(Opt, v ? 1L : 0L);
    }
};

template<auto Opt>
inline constexpr bool_opt_t<Opt> bool_opt;


template<auto Opt>
inline constexpr auto str_opt = [](auto&& v)
{
    if constexpr(_nullptr_<decltype(v)>)
    {
        return [](auto& h){ return h.setopt(Opt, static_cast<char*>(nullptr)); };
    }
    else if constexpr(_byte_str_<decltype(v)>)
    {
        return [vw = wrap_lref_fwd_other(DSK_FORWARD(v))](auto& h)
        {
            auto&& v = unwrap_ref(vw);

            if(str_empty(v)) return h.setopt(Opt, static_cast<char*>(nullptr));
            else             return h.setopt(Opt, const_cast<char*>(cstr_data<char>(as_cstr(v))));
        };
    }
    else
    {
        static_assert(false, "unsupported type");
    }
};

template<auto Opt, class T>
inline constexpr auto ptr_opt = [](T v)
{
    static_assert(std::is_pointer_v<T> || std::is_null_pointer_v<T>);

    return [v](auto& h){ return h.setopt(Opt, v); };
};

template<auto Opt>
inline constexpr auto data_opt = ptr_opt<Opt, void*>;

template<auto Opt, class T>
inline constexpr auto func_opt = [](T v)
{
    static_assert((std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>)
                  || std::is_null_pointer_v<T>);

    return [v](auto& h){ return h.setopt(Opt, v); };
};

template<auto Opt, auto DataOpt, class F>
inline constexpr auto cb_opt = [](F f, void* d)
{
    static_assert((std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>)
                  || std::is_null_pointer_v<F>);

    return [f, d](auto& h)
    {
        if(auto err = h.setopt(Opt, f))
        {
            return err;
        }

        return h.setopt(DataOpt, d);
    };
};

template<auto Opt, class Handle>
inline constexpr auto handle_opt = [](Handle& l)
{
    return [v = l.handle()](auto& h)
    {
        return h.setopt(Opt, v);
    };
};

template<auto Opt, class U, class Expected = long>
inline constexpr auto unit_opt = [](U const& u)
{
    return [u](auto& h)
    {
        return h.setopt(Opt, static_cast<Expected>(u.count()));
    };
};

template<auto Opt, class U>
inline constexpr auto lunit_opt = unit_opt<Opt, U, curl_off_t>;


// BEHAVIOR OPTIONS

inline constexpr auto verbose            = bool_opt<CURLOPT_VERBOSE      >;
inline constexpr auto header_to_write_cb = bool_opt<CURLOPT_HEADER       >;
inline constexpr auto noprogress         = bool_opt<CURLOPT_NOPROGRESS   >;
inline constexpr auto nosignal           = bool_opt<CURLOPT_NOSIGNAL     >;
inline constexpr auto wildcardmatch      = bool_opt<CURLOPT_WILDCARDMATCH>;


// CALLBACK OPTIONS

inline constexpr auto writefunc                = func_opt<CURLOPT_WRITEFUNCTION             , curl_write_callback         >;
inline constexpr auto writedata                = data_opt<CURLOPT_WRITEDATA                                               >;
inline constexpr auto readfunc                 = func_opt<CURLOPT_READFUNCTION              , curl_read_callback          >;
inline constexpr auto readdata                 = data_opt<CURLOPT_READDATA                                                >;
inline constexpr auto seekfunc                 = func_opt<CURLOPT_SEEKFUNCTION              , curl_seek_callback          >;
inline constexpr auto seekdata                 = data_opt<CURLOPT_SEEKDATA                                                >;
inline constexpr auto sockoptfunc              = func_opt<CURLOPT_SOCKOPTFUNCTION           , curl_sockopt_callback       >;
inline constexpr auto sockoptdata              = data_opt<CURLOPT_SOCKOPTDATA                                             >;
inline constexpr auto opensocketfunc           = func_opt<CURLOPT_OPENSOCKETFUNCTION        , curl_opensocket_callback    >;
inline constexpr auto opensocketdata           = data_opt<CURLOPT_OPENSOCKETDATA                                          >;
inline constexpr auto closesocketfunc          = func_opt<CURLOPT_CLOSESOCKETFUNCTION       , curl_closesocket_callback   >;
inline constexpr auto closesocketdata          = data_opt<CURLOPT_CLOSESOCKETDATA                                         >;
inline constexpr auto progressdata             = data_opt<CURLOPT_PROGRESSDATA                                            >;
inline constexpr auto xferinfofunc             = func_opt<CURLOPT_XFERINFOFUNCTION          , curl_xferinfo_callback      >;
inline constexpr auto xferinfodata             = data_opt<CURLOPT_XFERINFODATA                                            >;
inline constexpr auto headerfunc               = func_opt<CURLOPT_HEADERFUNCTION            , curl_write_callback         >;
inline constexpr auto headerdata               = data_opt<CURLOPT_HEADERDATA                                              >;
inline constexpr auto debugfunc                = func_opt<CURLOPT_DEBUGFUNCTION             , curl_debug_callback         >;
inline constexpr auto debugdata                = data_opt<CURLOPT_DEBUGDATA                                               >;
inline constexpr auto ssl_ctx_func             = func_opt<CURLOPT_SSL_CTX_FUNCTION          , curl_ssl_ctx_callback       >;
inline constexpr auto ssl_ctx_data             = data_opt<CURLOPT_SSL_CTX_DATA                                            >;
inline constexpr auto interleavefunc           = func_opt<CURLOPT_INTERLEAVEFUNCTION        , curl_write_callback         >;
inline constexpr auto interleavedata           = data_opt<CURLOPT_INTERLEAVEDATA                                          >;
inline constexpr auto chunk_bgn_func           = func_opt<CURLOPT_CHUNK_BGN_FUNCTION        , curl_chunk_bgn_callback     >;
inline constexpr auto chunk_end_func           = func_opt<CURLOPT_CHUNK_END_FUNCTION        , curl_chunk_end_callback     >;
inline constexpr auto chunk_data               = data_opt<CURLOPT_CHUNK_DATA                                              >;
inline constexpr auto suppress_connect_headers = bool_opt<CURLOPT_SUPPRESS_CONNECT_HEADERS                                >;
inline constexpr auto fnmatch_func             = func_opt<CURLOPT_FNMATCH_FUNCTION          , curl_fnmatch_callback       >;
inline constexpr auto fnmatch_data             = data_opt<CURLOPT_FNMATCH_DATA                                            >;
inline constexpr auto resolver_start_func      = func_opt<CURLOPT_RESOLVER_START_FUNCTION   , curl_resolver_start_callback>;
inline constexpr auto resolver_start_data      = data_opt<CURLOPT_RESOLVER_START_DATA                                     >;
inline constexpr auto prereq_func              = func_opt<CURLOPT_PREREQFUNCTION            , curl_prereq_callback        >;
inline constexpr auto prereq_data              = data_opt<CURLOPT_PREREQDATA                                              >;

inline constexpr auto write_cb          = cb_opt<CURLOPT_WRITEFUNCTION          , CURLOPT_WRITEDATA          , curl_write_callback         >;
inline constexpr auto read_cb           = cb_opt<CURLOPT_READFUNCTION           , CURLOPT_READDATA           , curl_read_callback          >;
inline constexpr auto seek_cb           = cb_opt<CURLOPT_SEEKFUNCTION           , CURLOPT_SEEKDATA           , curl_seek_callback          >;
inline constexpr auto sockopt_cb        = cb_opt<CURLOPT_SOCKOPTFUNCTION        , CURLOPT_SOCKOPTDATA        , curl_sockopt_callback       >;
inline constexpr auto opensocket_cb     = cb_opt<CURLOPT_OPENSOCKETFUNCTION     , CURLOPT_OPENSOCKETDATA     , curl_opensocket_callback    >;
inline constexpr auto closesocket_cb    = cb_opt<CURLOPT_CLOSESOCKETFUNCTION    , CURLOPT_CLOSESOCKETDATA    , curl_closesocket_callback   >;
inline constexpr auto xferinfo_cb       = cb_opt<CURLOPT_XFERINFOFUNCTION       , CURLOPT_XFERINFODATA       , curl_xferinfo_callback      >;
inline constexpr auto header_cb         = cb_opt<CURLOPT_HEADERFUNCTION         , CURLOPT_HEADERDATA         , curl_write_callback         >;
inline constexpr auto debug_cb          = cb_opt<CURLOPT_DEBUGFUNCTION          , CURLOPT_DEBUGDATA          , curl_debug_callback         >;
inline constexpr auto ssl_ctx_cb        = cb_opt<CURLOPT_SSL_CTX_FUNCTION       , CURLOPT_SSL_CTX_DATA       , curl_ssl_ctx_callback       >;
inline constexpr auto interleave_cb     = cb_opt<CURLOPT_INTERLEAVEFUNCTION     , CURLOPT_INTERLEAVEDATA     , curl_write_callback         >;
inline constexpr auto fnmatch_cb        = cb_opt<CURLOPT_FNMATCH_FUNCTION       , CURLOPT_FNMATCH_DATA       , curl_fnmatch_callback       >;
inline constexpr auto resolver_start_cb = cb_opt<CURLOPT_RESOLVER_START_FUNCTION, CURLOPT_RESOLVER_START_DATA, curl_resolver_start_callback>;

inline constexpr auto disbale_write_cb  = writefunc([](char*, size_t size, size_t nmemb, void*) -> size_t { return size * nmemb; });
inline constexpr auto disbale_header_cb = header_cb(static_cast<curl_write_callback>(nullptr), nullptr);

// ERROR OPTIONS

inline constexpr auto errbuf(_borrowed_byte_buf_ auto&& b)
{
    return [bw = wrap_lref_fwd_other(DSK_FORWARD(b))](auto& h)
    {
        auto&& b = unwrap_ref(bw);

        DSK_ASSERT(buf_size(b) >= CURL_ERROR_SIZE);

        return h.setopt(CURLOPT_ERRORBUFFER, buf_data<char>(b));
    };
}

inline constexpr auto std_err               =  ptr_opt<CURLOPT_STDERR, FILE*>;
inline constexpr auto keep_sending_on_error = bool_opt<CURLOPT_KEEP_SENDING_ON_ERROR>;

// an error code of CURLE_HTTP_RETURNED_ERROR will returned when HTTP response code >= 400,
// otherwise user may only get an invalid body with no error.
// NOTE: This option is not fail-safe and there are occasions where non-successful response codes
//       will slip through, especially when authentication is involved (response codes 401 and 407)
inline constexpr auto failonerror = bool_opt<CURLOPT_FAILONERROR>;


// NETWORK OPTIONS

inline constexpr auto url_ref = handle_opt<CURLOPT_CURLU, curlu>;
inline constexpr auto encoded_url = str_opt<CURLOPT_URL>; // assume the string is already url encoded

constexpr auto url(_byte_str_ auto&& u)
{
    return [uw = wrap_lref_fwd_other(DSK_FORWARD(u))](auto& h) -> error_code
    {
        DSK_E_TRY_FWD(s, encode_url(unwrap_ref(uw)));

        return h.setopt(CURLOPT_URL, const_cast<char*>(s.c_str()));
    };
}

constexpr auto url(std::nullptr_t)
{
    return [](auto& h)
    {
        return h.setopt(CURLOPT_URL, static_cast<char*>(nullptr));
    };
}


// urls can be a comma separated string or a range of string. 
constexpr auto noproxy(auto&& urls)
{
    if constexpr(requires{ requires _byte_str_<std::ranges::range_value_t<decltype(urls)>>; })
    {
        string l;

        for(auto const& u : urls)
        {
            l += u;
            l += ',';
        }

        return str_opt<CURLOPT_NOPROXY>(mut_move(urls));
    }
    else
    {
        return str_opt<CURLOPT_NOPROXY>(DSK_FORWARD(urls));
    }
}


inline constexpr auto path_as_is           = bool_opt<CURLOPT_PATH_AS_IS>;
inline constexpr auto protocols_str        =  str_opt<CURLOPT_PROTOCOLS_STR>;
inline constexpr auto redir_protocols_str  =  str_opt<CURLOPT_REDIR_PROTOCOLS_STR>;
inline constexpr auto default_protocol     =  str_opt<CURLOPT_DEFAULT_PROTOCOL>;
inline constexpr auto proxy                =  str_opt<CURLOPT_PROXY>;
inline constexpr auto pre_proxy            =  str_opt<CURLOPT_PRE_PROXY>;
inline constexpr auto proxyport            = long_opt<CURLOPT_PROXYPORT>;
inline constexpr auto proxytype            = long_opt<CURLOPT_PROXYTYPE>;
inline constexpr auto httpproxytunnel      = bool_opt<CURLOPT_HTTPPROXYTUNNEL>;
inline constexpr auto socks5_auth          = long_opt<CURLOPT_SOCKS5_AUTH>;
inline constexpr auto socks5_gssapi_nec    = bool_opt<CURLOPT_SOCKS5_GSSAPI_NEC>;
inline constexpr auto proxy_service_name   =  str_opt<CURLOPT_PROXY_SERVICE_NAME>;
inline constexpr auto haproxyprotocol      = long_opt<CURLOPT_HAPROXYPROTOCOL>;
inline constexpr auto service_name         =  str_opt<CURLOPT_SERVICE_NAME>;
inline constexpr auto interface_           =  str_opt<CURLOPT_INTERFACE>;
inline constexpr auto localport            = long_opt<CURLOPT_LOCALPORT>;
inline constexpr auto localportrange       = long_opt<CURLOPT_LOCALPORTRANGE>;
inline constexpr auto dns_cache_timeout    = long_opt<CURLOPT_DNS_CACHE_TIMEOUT>;
inline constexpr auto doh_url              =  str_opt<CURLOPT_DOH_URL>;
inline constexpr auto buffersize           = long_opt<CURLOPT_BUFFERSIZE>;
inline constexpr auto port                 = long_opt<CURLOPT_PORT>;
inline constexpr auto tcp_fastopen         = bool_opt<CURLOPT_TCP_FASTOPEN>;
inline constexpr auto tcp_nodelay          = bool_opt<CURLOPT_TCP_NODELAY>;
inline constexpr auto address_scope        = long_opt<CURLOPT_ADDRESS_SCOPE>;
inline constexpr auto tcp_keepalive        = bool_opt<CURLOPT_TCP_KEEPALIVE>;
inline constexpr auto tcp_keepidle         = unit_opt<CURLOPT_TCP_KEEPIDLE , std::chrono::seconds>;
inline constexpr auto tcp_keepintvl        = unit_opt<CURLOPT_TCP_KEEPINTVL, std::chrono::seconds>;
inline constexpr auto unix_socket_path     =  str_opt<CURLOPT_UNIX_SOCKET_PATH>;
inline constexpr auto abstract_unix_socket =  str_opt<CURLOPT_ABSTRACT_UNIX_SOCKET>;


// NAMES and PASSWORDS OPTIONS (Authentication)

inline constexpr auto netrc                    = bool_opt<CURLOPT_NETRC                   >;
inline constexpr auto netrc_file               =  str_opt<CURLOPT_NETRC_FILE              >;
inline constexpr auto userpwd                  =  str_opt<CURLOPT_USERPWD                 >;
inline constexpr auto proxyuserpwd             =  str_opt<CURLOPT_PROXYUSERPWD            >;
inline constexpr auto username                 =  str_opt<CURLOPT_USERNAME                >;
inline constexpr auto password                 =  str_opt<CURLOPT_PASSWORD                >;
inline constexpr auto login_options            =  str_opt<CURLOPT_LOGIN_OPTIONS           >;
inline constexpr auto proxyusername            =  str_opt<CURLOPT_PROXYUSERNAME           >;
inline constexpr auto proxypassword            =  str_opt<CURLOPT_PROXYPASSWORD           >;
inline constexpr auto httpauth                 = long_opt<CURLOPT_HTTPAUTH                >;
inline constexpr auto tlsauth_username         =  str_opt<CURLOPT_TLSAUTH_USERNAME        >;
inline constexpr auto proxy_tlsauth_username   =  str_opt<CURLOPT_PROXY_TLSAUTH_USERNAME  >;
inline constexpr auto tlsauth_password         =  str_opt<CURLOPT_TLSAUTH_PASSWORD        >;
inline constexpr auto proxy_tlsauth_password   =  str_opt<CURLOPT_PROXY_TLSAUTH_PASSWORD  >;
inline constexpr auto tlsauth_type             =  str_opt<CURLOPT_TLSAUTH_TYPE            >;
inline constexpr auto proxy_tlsauth_type       =  str_opt<CURLOPT_PROXY_TLSAUTH_TYPE      >;
inline constexpr auto proxyauth                = long_opt<CURLOPT_PROXYAUTH               >;
inline constexpr auto sasl_authzid             =  str_opt<CURLOPT_SASL_AUTHZID            >;
inline constexpr auto sasl_ir                  = bool_opt<CURLOPT_SASL_IR                 >;
inline constexpr auto xoauth2_bearer           =  str_opt<CURLOPT_XOAUTH2_BEARER          >;
inline constexpr auto disallow_username_in_url = bool_opt<CURLOPT_DISALLOW_USERNAME_IN_URL>;


// HTTP OPTIONS

inline constexpr auto auto_referer      = bool_opt<CURLOPT_AUTOREFERER      >;
inline constexpr auto accept_encoding   =  str_opt<CURLOPT_ACCEPT_ENCODING  >;
inline constexpr auto transfer_encoding = bool_opt<CURLOPT_TRANSFER_ENCODING>;
inline constexpr auto follow_location   = bool_opt<CURLOPT_FOLLOWLOCATION   >;
inline constexpr auto unrestricted_auth = bool_opt<CURLOPT_UNRESTRICTED_AUTH>;
inline constexpr auto max_redirs        = long_opt<CURLOPT_MAXREDIRS        >;
inline constexpr auto post_redir        = long_opt<CURLOPT_POSTREDIR        >;

inline constexpr auto customrequest     =  str_opt<CURLOPT_CUSTOMREQUEST    >; // change the http verb, but keep the underlying behavior.
inline constexpr auto httpget           = bool_opt<CURLOPT_HTTPGET          >; // GET (the default option), no request body.
inline constexpr auto nobody            = bool_opt<CURLOPT_NOBODY           >; // HEAD request, no request/response body.
inline constexpr auto post              = bool_opt<CURLOPT_POST             >; // POST, can have request body, set "Expect: 100-continue" when body size > 1kb for HTTP 1.1 and set "Content-Type: application/x-www-form-urlencoded" by default

// this option also sets CURLOPT_POST.
constexpr auto postfields(_borrowed_byte_buf_ auto&& b)
{
    return [bw = wrap_lref_fwd_other(DSK_FORWARD(b))](auto& h)
    {
        auto&& b = unwrap_ref(bw);

        if(auto err = h.setopt(CURLOPT_POSTFIELDSIZE_LARGE, buf_size<curl_off_t>(b)))
        {
            return err;
        }

        return h.setopt(CURLOPT_POSTFIELDS, const_cast<char*>(buf_data<char>(b)));
    };
}

constexpr auto postfields(std::nullptr_t) // reset to default
{
    return [](auto& h)
    {
        if(auto err = h.setopt(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(0)))
        {
            return err;
        }

        return h.setopt(CURLOPT_POSTFIELDS, static_cast<char*>(nullptr));
    };
}

// implies CURLOPT_POST.
constexpr auto copypostfields(_byte_buf_ auto const& b)
{
    return [bw = wrap_lref_fwd_other(DSK_FORWARD(b))](auto& h)
    {
        auto&& b = unwrap_ref(bw);

        if(auto err = h.setopt(CURLOPT_POSTFIELDSIZE_LARGE, buf_size<curl_off_t>(b)))
        {
            return err;
        }

        return h.setopt(CURLOPT_COPYPOSTFIELDS, const_cast<char*>(buf_data<char>(b)));
    };
}

inline constexpr auto referer                =    str_opt<CURLOPT_REFERER                                           >;
inline constexpr auto useragent              =    str_opt<CURLOPT_USERAGENT                                         >;
inline constexpr auto httpheader             = handle_opt<CURLOPT_HTTPHEADER            , curlist                   >;
inline constexpr auto headeropt              =   long_opt<CURLOPT_HEADEROPT                                         >;
inline constexpr auto proxyheader            = handle_opt<CURLOPT_PROXYHEADER           , curlist                   >;
inline constexpr auto http200aliases         = handle_opt<CURLOPT_HTTP200ALIASES        , curlist                   >;
inline constexpr auto cookie                 =    str_opt<CURLOPT_COOKIE                                            >;
inline constexpr auto cookiefile             =    str_opt<CURLOPT_COOKIEFILE                                        >;
inline constexpr auto cookiejar              =    str_opt<CURLOPT_COOKIEJAR                                         >;
inline constexpr auto cookiesession          =   bool_opt<CURLOPT_COOKIESESSION                                     >;
inline constexpr auto cookielist             =    str_opt<CURLOPT_COOKIELIST                                        >;
inline constexpr auto altsvc                 =    str_opt<CURLOPT_ALTSVC                                            >;
inline constexpr auto altsvc_ctrl            =   long_opt<CURLOPT_ALTSVC_CTRL                                       >;
inline constexpr auto request_target         =    str_opt<CURLOPT_REQUEST_TARGET                                    >;
inline constexpr auto http_version           =   long_opt<CURLOPT_HTTP_VERSION                                      >;
inline constexpr auto http09_allowed         =   bool_opt<CURLOPT_HTTP09_ALLOWED                                    >;
inline constexpr auto ignore_content_length  =   bool_opt<CURLOPT_IGNORE_CONTENT_LENGTH                             >;
inline constexpr auto http_content_decoding  =   bool_opt<CURLOPT_HTTP_CONTENT_DECODING                             >;
inline constexpr auto http_transfer_decoding =   bool_opt<CURLOPT_HTTP_TRANSFER_DECODING                            >;
inline constexpr auto expect_100_timeout     =   unit_opt<CURLOPT_EXPECT_100_TIMEOUT_MS  , std::chrono::milliseconds>;
inline constexpr auto trailerfunction        =   func_opt<CURLOPT_TRAILERFUNCTION        , curl_trailer_callback    >;
inline constexpr auto trailerdata            =   data_opt<CURLOPT_TRAILERDATA                                       >;
inline constexpr auto pipewait               =   bool_opt<CURLOPT_PIPEWAIT                                          >;
inline constexpr auto stream_depends         =    ptr_opt<CURLOPT_STREAM_DEPENDS         , CURL*                    >;
inline constexpr auto stream_depends_e       =    ptr_opt<CURLOPT_STREAM_DEPENDS_E       , CURL*                    >;
inline constexpr auto stream_weight          =   long_opt<CURLOPT_STREAM_WEIGHT                                     >;


// SMTP OPTIONS

inline constexpr auto mail_from            =    str_opt<CURLOPT_MAIL_FROM                     >;
inline constexpr auto mail_rcpt            = handle_opt<CURLOPT_MAIL_RCPT            , curlist>;
inline constexpr auto mail_auth            =    str_opt<CURLOPT_MAIL_AUTH                     >;
inline constexpr auto mail_rcpt_allowfails =   bool_opt<CURLOPT_MAIL_RCPT_ALLOWFAILS          >;


// TFTP OPTIONS

inline constexpr auto tftp_blksize    = long_opt<CURLOPT_TFTP_BLKSIZE   >;
inline constexpr auto tftp_no_options = bool_opt<CURLOPT_TFTP_NO_OPTIONS>;


// FTP OPTIONS

inline constexpr auto ftpport                 =    str_opt<CURLOPT_FTPPORT                                              >;
inline constexpr auto quote                   = handle_opt<CURLOPT_QUOTE                     , curlist                  >;
inline constexpr auto postquote               = handle_opt<CURLOPT_POSTQUOTE                 , curlist                  >;
inline constexpr auto prequote                = handle_opt<CURLOPT_PREQUOTE                  , curlist                  >;
inline constexpr auto append                  =   bool_opt<CURLOPT_APPEND                                               >;
inline constexpr auto ftp_use_eprt            =   bool_opt<CURLOPT_FTP_USE_EPRT                                         >;
inline constexpr auto ftp_use_epsv            =   bool_opt<CURLOPT_FTP_USE_EPSV                                         >;
inline constexpr auto ftp_use_pret            =   bool_opt<CURLOPT_FTP_USE_PRET                                         >;
inline constexpr auto ftp_create_missing_dirs =   long_opt<CURLOPT_FTP_CREATE_MISSING_DIRS                              >;
inline constexpr auto ftp_response_timeout    =   unit_opt<CURLOPT_FTP_RESPONSE_TIMEOUT      , std::chrono::seconds     >;
inline constexpr auto ftp_alternative_to_user =    str_opt<CURLOPT_FTP_ALTERNATIVE_TO_USER                              >;
inline constexpr auto ftp_skip_pasv_ip        =   bool_opt<CURLOPT_FTP_SKIP_PASV_IP                                     >;
inline constexpr auto ftpsslauth              =   long_opt<CURLOPT_FTPSSLAUTH                                           >;
inline constexpr auto ftp_ssl_ccc             =   long_opt<CURLOPT_FTP_SSL_CCC                                          >;
inline constexpr auto ftp_account             =    str_opt<CURLOPT_FTP_ACCOUNT                                          >;
inline constexpr auto ftp_filemethod          =   long_opt<CURLOPT_FTP_FILEMETHOD                                       >;
inline constexpr auto server_response_timeout =   unit_opt<CURLOPT_SERVER_RESPONSE_TIMEOUT_MS, std::chrono::milliseconds>;


// RTSP OPTIONS

inline constexpr auto rtsp_request     = long_opt<CURLOPT_RTSP_REQUEST    >;
inline constexpr auto rtsp_session_id  =  str_opt<CURLOPT_RTSP_SESSION_ID >;
inline constexpr auto rtsp_stream_uri  =  str_opt<CURLOPT_RTSP_STREAM_URI >;
inline constexpr auto rtsp_transport   =  str_opt<CURLOPT_RTSP_TRANSPORT  >;
inline constexpr auto rtsp_client_cseq = long_opt<CURLOPT_RTSP_CLIENT_CSEQ>;
inline constexpr auto rtsp_server_cseq = long_opt<CURLOPT_RTSP_SERVER_CSEQ>;


// PROTOCOL OPTIONS

inline constexpr auto transfertext        = bool_opt<CURLOPT_TRANSFERTEXT                    >;
inline constexpr auto proxy_transfer_mode = bool_opt<CURLOPT_PROXY_TRANSFER_MODE             >;
inline constexpr auto crlf                = bool_opt<CURLOPT_CRLF                            >;
inline constexpr auto range               =  str_opt<CURLOPT_RANGE                           >;
inline constexpr auto resume_from         = offt_opt<CURLOPT_RESUME_FROM_LARGE               >;
inline constexpr auto filetime            = bool_opt<CURLOPT_FILETIME                        >;
inline constexpr auto dirlistonly         = bool_opt<CURLOPT_DIRLISTONLY                     >;
inline constexpr auto infilesize          = offt_opt<CURLOPT_INFILESIZE_LARGE                >;
inline constexpr auto upload              = bool_opt<CURLOPT_UPLOAD                          >;
inline constexpr auto upload_buffersize   = long_opt<CURLOPT_UPLOAD_BUFFERSIZE               >;
inline constexpr auto mimepost            =  ptr_opt<CURLOPT_MIMEPOST            , curl_mime*>;
inline constexpr auto maxfilesize         = offt_opt<CURLOPT_MAXFILESIZE_LARGE               >;
inline constexpr auto timecondition       = long_opt<CURLOPT_TIMECONDITION                   >;
inline constexpr auto timevalue           = long_opt<CURLOPT_TIMEVALUE                       >;
inline constexpr auto timevalue_large     = offt_opt<CURLOPT_TIMEVALUE_LARGE                 >;


// CONNECTION OPTIONS

inline constexpr auto timeout                =   unit_opt<CURLOPT_TIMEOUT_MS               , std::chrono::milliseconds>;
inline constexpr auto low_speed_limit        =   unit_opt<CURLOPT_LOW_SPEED_LIMIT          , B_                       >;
inline constexpr auto low_speed_time         =   unit_opt<CURLOPT_LOW_SPEED_TIME           , std::chrono::seconds     >;
inline constexpr auto max_send_speed         =  lunit_opt<CURLOPT_MAX_SEND_SPEED_LARGE     , B_                       >;
inline constexpr auto max_recv_speed         =  lunit_opt<CURLOPT_MAX_RECV_SPEED_LARGE     , B_                       >;
inline constexpr auto max_connects           =   long_opt<CURLOPT_MAXCONNECTS                                         >;
inline constexpr auto fresh_connect          =   bool_opt<CURLOPT_FRESH_CONNECT                                       >;
inline constexpr auto forbid_reuse           =   bool_opt<CURLOPT_FORBID_REUSE                                        >;
inline constexpr auto maxage_conn            =   unit_opt<CURLOPT_MAXAGE_CONN              , std::chrono::seconds     >;
inline constexpr auto maxlifetime_conn       =   unit_opt<CURLOPT_MAXLIFETIME_CONN         , std::chrono::seconds     >;
inline constexpr auto connect_timeout        =   unit_opt<CURLOPT_CONNECTTIMEOUT_MS        , std::chrono::milliseconds>;
inline constexpr auto ipresolve              =   long_opt<CURLOPT_IPRESOLVE                                           >;
inline constexpr auto connect_only           =   bool_opt<CURLOPT_CONNECT_ONLY                                        >;
inline constexpr auto use_ssl                =   long_opt<CURLOPT_USE_SSL                                             >;
inline constexpr auto resolve                = handle_opt<CURLOPT_RESOLVE                  , curlist                  >;
inline constexpr auto dns_interface          =    str_opt<CURLOPT_DNS_INTERFACE                                       >;
inline constexpr auto dns_local_ip4          =    str_opt<CURLOPT_DNS_LOCAL_IP4                                       >;
inline constexpr auto dns_local_ip6          =    str_opt<CURLOPT_DNS_LOCAL_IP6                                       >;
inline constexpr auto dns_servers            =    str_opt<CURLOPT_DNS_SERVERS                                         >;
inline constexpr auto dns_shuffle_addresses  =   bool_opt<CURLOPT_DNS_SHUFFLE_ADDRESSES                               >;
inline constexpr auto accept_timeout         =   unit_opt<CURLOPT_ACCEPTTIMEOUT_MS         , std::chrono::milliseconds>;
inline constexpr auto happy_eyeballs_timeout =   unit_opt<CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, std::chrono::milliseconds>;


// SSL and SECURITY OPTIONS

inline constexpr auto sslcert               =  str_opt<CURLOPT_SSLCERT              >;
inline constexpr auto sslcert_blob          =  str_opt<CURLOPT_SSLCERT_BLOB         >;
inline constexpr auto proxy_sslcert         =  str_opt<CURLOPT_PROXY_SSLCERT        >;
inline constexpr auto proxy_sslcert_blob    =  str_opt<CURLOPT_PROXY_SSLCERT_BLOB   >;
inline constexpr auto sslcerttype           =  str_opt<CURLOPT_SSLCERTTYPE          >;
inline constexpr auto proxy_sslcerttype     =  str_opt<CURLOPT_PROXY_SSLCERTTYPE    >;
inline constexpr auto sslkey                =  str_opt<CURLOPT_SSLKEY               >;
inline constexpr auto sslkey_blob           =  str_opt<CURLOPT_SSLKEY_BLOB          >;
inline constexpr auto proxy_sslkey          =  str_opt<CURLOPT_PROXY_SSLKEY         >;
inline constexpr auto proxy_sslkey_blob     =  str_opt<CURLOPT_PROXY_SSLKEY_BLOB    >;
inline constexpr auto sslkeytype            =  str_opt<CURLOPT_SSLKEYTYPE           >;
inline constexpr auto proxy_sslkeytype      =  str_opt<CURLOPT_PROXY_SSLKEYTYPE     >;
inline constexpr auto keypasswd             =  str_opt<CURLOPT_KEYPASSWD            >;
inline constexpr auto proxy_keypasswd       =  str_opt<CURLOPT_PROXY_KEYPASSWD      >;
inline constexpr auto ssl_ec_curves         =  str_opt<CURLOPT_SSL_EC_CURVES        >;
inline constexpr auto ssl_enable_alpn       = bool_opt<CURLOPT_SSL_ENABLE_ALPN      >;
inline constexpr auto sslengine             =  str_opt<CURLOPT_SSLENGINE            >;
inline constexpr auto sslengine_default     = bool_opt<CURLOPT_SSLENGINE_DEFAULT    >;
inline constexpr auto ssl_falsestart        = bool_opt<CURLOPT_SSL_FALSESTART       >;
inline constexpr auto sslversion            = long_opt<CURLOPT_SSLVERSION           >;
inline constexpr auto proxy_sslversion      = long_opt<CURLOPT_PROXY_SSLVERSION     >;
inline constexpr auto ssl_verifyhost        = long_opt<CURLOPT_SSL_VERIFYHOST       >;
inline constexpr auto proxy_ssl_verifyhost  = long_opt<CURLOPT_PROXY_SSL_VERIFYHOST >;
inline constexpr auto ssl_verifypeer        = bool_opt<CURLOPT_SSL_VERIFYPEER       >;
inline constexpr auto proxy_ssl_verifypeer  = bool_opt<CURLOPT_PROXY_SSL_VERIFYPEER >;
inline constexpr auto ssl_verifystatus      = bool_opt<CURLOPT_SSL_VERIFYSTATUS     >;
inline constexpr auto cainfo                =  str_opt<CURLOPT_CAINFO               >;
inline constexpr auto cainfo_blob           =  ptr_opt<CURLOPT_CAINFO_BLOB          , curl_blob*>;
inline constexpr auto proxy_cainfo          =  str_opt<CURLOPT_PROXY_CAINFO         >;
inline constexpr auto proxy_cainfo_blob     =  ptr_opt<CURLOPT_PROXY_CAINFO_BLOB    , curl_blob*>;
inline constexpr auto issuercert            =  str_opt<CURLOPT_ISSUERCERT           >;
inline constexpr auto issuercert_blob       =  str_opt<CURLOPT_ISSUERCERT_BLOB      >;
inline constexpr auto proxy_issuercert      =  str_opt<CURLOPT_PROXY_ISSUERCERT     >;
inline constexpr auto proxy_issuercert_blob =  str_opt<CURLOPT_PROXY_ISSUERCERT_BLOB>;
inline constexpr auto capath                =  str_opt<CURLOPT_CAPATH               >;
inline constexpr auto proxy_capath          =  str_opt<CURLOPT_PROXY_CAPATH         >;
inline constexpr auto crlfile               =  str_opt<CURLOPT_CRLFILE              >;
inline constexpr auto proxy_crlfile         =  str_opt<CURLOPT_PROXY_CRLFILE        >;
inline constexpr auto certinfo              = bool_opt<CURLOPT_CERTINFO             >;
inline constexpr auto pinnedpublickey       =  str_opt<CURLOPT_PINNEDPUBLICKEY      >;
inline constexpr auto proxy_pinnedpublickey =  str_opt<CURLOPT_PROXY_PINNEDPUBLICKEY>;
inline constexpr auto ssl_cipher_list       =  str_opt<CURLOPT_SSL_CIPHER_LIST      >;
inline constexpr auto proxy_ssl_cipher_list =  str_opt<CURLOPT_PROXY_SSL_CIPHER_LIST>;
inline constexpr auto tls13_ciphers         =  str_opt<CURLOPT_TLS13_CIPHERS        >;
inline constexpr auto proxy_tls13_ciphers   =  str_opt<CURLOPT_PROXY_TLS13_CIPHERS  >;
inline constexpr auto ssl_sessionid_cache   = bool_opt<CURLOPT_SSL_SESSIONID_CACHE  >;
inline constexpr auto ssl_options           = long_opt<CURLOPT_SSL_OPTIONS          >;
inline constexpr auto proxy_ssl_options     = long_opt<CURLOPT_PROXY_SSL_OPTIONS    >;
inline constexpr auto krblevel              =  str_opt<CURLOPT_KRBLEVEL             >;
inline constexpr auto gssapi_delegation     = long_opt<CURLOPT_GSSAPI_DELEGATION    >;


// SSH OPTIONS

inline constexpr auto ssh_auth_types          = long_opt<CURLOPT_SSH_AUTH_TYPES                                  >;
inline constexpr auto ssh_compression         = bool_opt<CURLOPT_SSH_COMPRESSION                                 >;
inline constexpr auto ssh_host_public_key_md5 =  str_opt<CURLOPT_SSH_HOST_PUBLIC_KEY_MD5                         >;
inline constexpr auto ssh_public_keyfile      =  str_opt<CURLOPT_SSH_PUBLIC_KEYFILE                              >;
inline constexpr auto ssh_private_keyfile     =  str_opt<CURLOPT_SSH_PRIVATE_KEYFILE                             >;
inline constexpr auto ssh_knownhosts          =  str_opt<CURLOPT_SSH_KNOWNHOSTS                                  >;
inline constexpr auto ssh_keyfunction         = func_opt<CURLOPT_SSH_KEYFUNCTION        , curl_sshkeycallback    >;
inline constexpr auto ssh_keydata             = data_opt<CURLOPT_SSH_KEYDATA                                     >;
inline constexpr auto ssh_hostkeyfunction     = func_opt<CURLOPT_SSH_HOSTKEYFUNCTION    , curl_sshhostkeycallback>;
inline constexpr auto ssh_hostkeydata         = data_opt<CURLOPT_SSH_HOSTKEYDATA                                 >;


// OTHER OPTIONS

inline constexpr auto priv_data           =   data_opt<CURLOPT_PRIVATE                        >;
inline constexpr auto share               = handle_opt<CURLOPT_SHARE              , curl_share>;
inline constexpr auto new_file_perms      =   long_opt<CURLOPT_NEW_FILE_PERMS                 >;
inline constexpr auto new_directory_perms =   long_opt<CURLOPT_NEW_DIRECTORY_PERMS            >;
inline constexpr auto quick_exit          =   bool_opt<CURLOPT_QUICK_EXIT                     >;


// TELNET OPTIONS

inline constexpr auto telnetoptions = handle_opt<CURLOPT_TELNETOPTIONS, curlist>;


} // namespace dsk::curlopt



namespace dsk::curlmopt{


inline constexpr auto chunk_length_penalty_size   = curlopt::long_opt<CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE                             >;
inline constexpr auto content_length_penalty_size = curlopt::long_opt<CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE                           >;
inline constexpr auto max_connections             = curlopt::long_opt<CURLMOPT_MAX_TOTAL_CONNECTIONS                                 >;
inline constexpr auto max_connection_cache        = curlopt::long_opt<CURLMOPT_MAXCONNECTS                                           >;
inline constexpr auto max_per_host_connections    = curlopt::long_opt<CURLMOPT_MAX_HOST_CONNECTIONS                                  >;
inline constexpr auto max_pipeline_length         = curlopt::long_opt<CURLMOPT_MAX_PIPELINE_LENGTH                                   >;
inline constexpr auto pipelining                  = curlopt::long_opt<CURLMOPT_PIPELINING                                            >;
inline constexpr auto pushfunction                = curlopt::func_opt<CURLMOPT_PUSHFUNCTION               , curl_push_callback       >;
inline constexpr auto pushdata                    = curlopt::data_opt<CURLMOPT_PUSHDATA                                              >;
inline constexpr auto socketfunction              = curlopt::func_opt<CURLMOPT_SOCKETFUNCTION             , curl_socket_callback     >;
inline constexpr auto socketdata                  = curlopt::data_opt<CURLMOPT_SOCKETDATA                                            >;
inline constexpr auto timerfunction               = curlopt::func_opt<CURLMOPT_TIMERFUNCTION              , curl_multi_timer_callback>;
inline constexpr auto timerdata                   = curlopt::data_opt<CURLMOPT_TIMERDATA                                             >;
inline constexpr auto max_concurrent_streams      = curlopt::long_opt<CURLMOPT_MAX_CONCURRENT_STREAMS                                >;


inline constexpr auto push_cb   = curlopt::cb_opt<CURLMOPT_PUSHFUNCTION  , CURLMOPT_PUSHDATA  , curl_push_callback       >;
inline constexpr auto socket_cb = curlopt::cb_opt<CURLMOPT_SOCKETFUNCTION, CURLMOPT_SOCKETDATA, curl_socket_callback     >;
inline constexpr auto timer_cb  = curlopt::cb_opt<CURLMOPT_TIMERFUNCTION , CURLMOPT_TIMERDATA , curl_multi_timer_callback>;


} // namespace dsk::curlmopt


namespace dsk::curlshopt{


inline constexpr auto lockfunc   = curlopt::func_opt<CURLSHOPT_LOCKFUNC  , curl_lock_function  >;
inline constexpr auto unlockfunc = curlopt::func_opt<CURLSHOPT_UNLOCKFUNC, curl_unlock_function>;
inline constexpr auto userdata   = curlopt::data_opt<CURLSHOPT_USERDATA                        >;
inline constexpr auto share      = curlopt::long_opt<CURLSHOPT_SHARE                           >;
inline constexpr auto unshare    = curlopt::long_opt<CURLSHOPT_UNSHARE                         >;


} // namespace dsk::curlshopt
