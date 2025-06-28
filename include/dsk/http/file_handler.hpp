#pragma once

#include <dsk/task.hpp>
#include <dsk/util/string.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/util/filesystem.hpp>
#include <dsk/http/url.hpp>
#include <dsk/http/conn.hpp>
#include <dsk/http/mime.hpp>
#include <dsk/http/range_header.hpp>
#include <dsk/asio/send_file.hpp>


namespace dsk{


class http_file_handler
{
    std::filesystem::path _root;
    std::filesystem::path _defaultFile = "index.html";
    http_fields _commonFields;
    string _cacheControl = "public, max-age=15552000"; // half year

    http_response make_res(_http_request_ auto const& req, http_status status, char const* reason = "") const
    {
        http_response res(status, req.version(), reason, _commonFields);

        res.keep_alive(req.keep_alive());
        
        if(res.body().size())
        {
            res.set(http_field::content_type, "text/html");
            res.prepare_payload();
        }

        return res;
    }

    template<class S = string>
    S make_etag(auto&& path, size_t fileSize) const
    {
        std::error_code ec;
        auto mTime = std::filesystem::last_write_time(path, ec).time_since_epoch();

        if(! ec)
        {
            return cat_as_str<S>('"', with_radix<16>(mTime.count()), '-', with_radix<16>(fileSize), '"');
        }

        return S();
    }

public:
    http_file_handler() = default;
    
    explicit http_file_handler(auto&& root)
        : _root(DSK_FORWARD(root))
    {}
    
    http_file_handler(auto&& root, auto&& defaultFile)
        : _root(DSK_FORWARD(root)), _defaultFile(DSK_FORWARD(defaultFile))
    {}

    std::filesystem::path const& root() const noexcept { return _root; }
    std::filesystem::path const& default_file() const noexcept { return _defaultFile; }
    http_fields const& common_fields() const noexcept { return _commonFields; }
    string const& cache_control() const noexcept { return _cacheControl; }

    void set_root(auto&& path)
    {
        _root = DSK_FORWARD(path);
    }

    void set_default_file(auto&& fileName)
    {
        _defaultFile = DSK_FORWARD(fileName);
    }

    void set_common_fields(_http_fields_ auto&& fields)
    {
        _commonFields = DSK_FORWARD(fields);
    }

    void set_cache_control_str(_byte_str_ auto&& s)
    {
        assign_str(_cacheControl, DSK_FORWARD(s));
    }

    void set_cache_control(_byte_str_ auto&&... directives)
    {
        _cacheControl.clear();

        if(sizeof...(directives))
        {
            (..., (append_str(_cacheControl, DSK_FORWARD(directives)), _cacheControl += ", "));
            _cacheControl.resize(_cacheControl.size() - 2);
        }
    }

    void set_cache_control(std::chrono::seconds const& maxAge, _byte_str_ auto&&... directives)
    {
        set_cache_control(cat_as_str("max-age=", maxAge.count()), DSK_FORWARD(directives)...);
    }

    // Usually, this should be the last handler,
    // and if this function failed, one can return 500 Internal Server Error as generic "catch-all" response.
    // and if ! req.keep_alive(), close the conn.
    template<class Socket>
    task<> handle_request(http_conn_t<Socket>& conn, _http_request_ auto& req) const
    {
        if(! is_oneof(req.method(), http_verb::get, http_verb::head))
        {
            DSK_TRY conn.write(make_res(req, http_status::bad_request, "Unknown HTTP-method"));
            DSK_RETURN();
        }

        string target;
        {
            if(req.target().empty() ||
               req.target()[0] != '/' ||
               req.target().contains("..") ||
               has_err(percent_decode(req.target(), target)))
            {
                DSK_TRY conn.write(make_res(req, http_status::bad_request, "Illegal request-target"));
                DSK_RETURN();
            }
        }

        auto path = _root / str_view<char8_t>(target).substr(1);
        {
            if(target.back() == '/')
            {
                path /= _defaultFile;
            }
        }

        stream_file file;
        {                                        //vvvv: asio on windows uses CreateFileA which only supports ansi path.
            auto const ec = file.open_ro(to_str_of<char>(path));

            if(ec == sys_errc::no_such_file_or_directory)
            {
                DSK_TRY conn.write(make_res(req, http_status::not_found, "Target not found"));
                DSK_RETURN();
            }

            DSK_TRY_SYNC ec;
        }

        size_t fileSize = DSK_TRY_SYNC file.size();

        auto st = http_status::ok;
        string etag;
        bool etagTried = false;
        http_range range;
        {
            if(auto it = req.find(http_field::if_none_match); it != req.end())
            {
                // Cache-Control decides if a resource is fresh or stale from response time.
                // If stale, a Get/Head request with `If-None-Match: etag` is issued.
                // If etag is same, return `304 Not Modified` response(no body).
                // Otherwise return new content.
                // https://developer.mozilla.org/en-US/docs/Web/HTTP/Caching#etagif-none-match

                etag = make_etag(path, fileSize);
                etagTried = true;

                if(etag.size() && it->value() == etag)
                {
                    st = http_status::not_modified;
                }
            }
            else if(auto it = req.find(http_field::range); it != req.end())
            {
                auto r = parse_single_range_header(it->value());

                if(has_err(r))
                {
                    if(get_err(r) == errc::out_of_bound)
                    {
                        DSK_TRY conn.write(make_res(req, http_status::range_not_satisfiable));
                    }
                    else
                    {
                        DSK_TRY conn.write(make_res(req, http_status::bad_request, "Unsupported range syntax"));
                    }

                    DSK_RETURN();
                }

                r = get_val(r).length_to_range(fileSize);
            
                if(has_err(r))
                {
                    DSK_TRY conn.write(make_res(req, http_status::range_not_satisfiable));
                    DSK_RETURN();
                }

                range = get_val(r);
                st = http_status::partial_content;
            }
        }

        http_response res(st, req.version(), "", _commonFields);
        {
            res.keep_alive(req.keep_alive());
            res.set(http_field::accept_ranges, "bytes");
            res.set(http_field::content_type, ext_to_mime(to_str_of<char>(path.extension())));

            if(st == http_status::partial_content)
            {
                res.set(http_field::content_range, cat_as_str("bytes ", range.first_byte, "-", range.last_byte, "/", fileSize));
                res.content_length(range.last_byte - range.first_byte + 1);
            }
            else
            {
                res.content_length(fileSize);
                res.set(http_field::cache_control, _cacheControl);

                if(! etagTried)
                {
                    etag = make_etag(path, fileSize);
                }

                if(etag.size())
                {
                    res.set(http_field::etag, etag);
                }
            }
        }

        http_response_serializer sr(res);
        DSK_TRY conn.write_header(sr);

        if(st != http_status::not_modified && req.method() == http_verb::get)
        {
            if(st == http_status::partial_content)
            {
                DSK_TRY send_file(conn, file, {.offset = static_cast<size_t>(range.first_byte),
                                               .count  = static_cast<size_t>(range.last_byte - range.first_byte + 1)});
            }
            else
            {
                DSK_TRY send_file(conn, file);
            }
        }

        DSK_RETURN();
    }
};


} // namespace dsk
