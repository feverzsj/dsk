#pragma once

#include <dsk/compr/bz2.hpp>
#include <dsk/compr/zlib.hpp>
#include <dsk/compr/zstd.hpp>
#include <dsk/util/variant.hpp>
#include <dsk/util/flat_map.hpp>


namespace dsk{


class auto_decompressor
{
    enum decompr_e{     unknown,              bz2,              zlib,              zstd };
    std::variant<std::monostate, bz2_decompressor, zlib_decompressor, zstd_decompressor> _decompr;
    bool _inited = false;
    bz2_decompr_opts _bz2Opts;
    zlib_decompr_opts _zlibOpts;
    flat_map<ZSTD_dParameter, int> _zstdOpts;

    static constexpr decompr_e detect_type(_byte_str_ auto const& s) noexcept
    {
        auto&& v = str_view<char>(s);

        if(v.starts_with("\x42\x5A\x68"))
        {
            return bz2;
        }

        if(    v.starts_with("\x1F\x8B") // gzip
            || v.starts_with("\x78\x01")
            || v.starts_with("\x78\x9C")
            || v.starts_with("\x78\xDA"))
        {
            return zlib;
        }

        if(v.starts_with("\x28\xB5\x2F\xFD"))
        {
            return zstd;
        }

        return unknown;
    }

    error_code init(_byte_buf_ auto const& in)
    {
        if(_inited)
        {
            if(_decompr.index() == unknown) return errc::invalid_input;
            else                            return {};
        }

        DSK_ASSERT(_decompr.index() == unknown);

        decompr_e t = detect_type(in);

        switch(t)
        {
            case bz2 : { DSK_E_TRY_ONLY(_decompr.template emplace<bz2 >().reinit( _bz2Opts)); } break;
            case zlib: { DSK_E_TRY_ONLY(_decompr.template emplace<zlib>().reinit(_zlibOpts)); } break;
            case zstd:
                {
                    zstd_decompressor& decompr = _decompr.template emplace<zstd>();

                    for(auto&[k, v] : _zstdOpts)
                    {
                        DSK_E_TRY_ONLY(decompr.set(k, v));
                    }
                }
                break;
            default:
                break;
        }

        _inited = true;
        return {};
    }

public:
    void reset()
    {
        _decompr.emplace<unknown>();
        _inited = false;
    }

    void set_bz2_opts(bz2_decompr_opts const& opts) noexcept
    {
        _bz2Opts = opts;
    }

    void set_zlib_opts(zlib_decompr_opts const& opts) noexcept
    {
        _zlibOpts = opts;
    }

    void set_zstd_opts(ZSTD_dParameter k, int v, auto... kvs) noexcept
    {
        _zstdOpts[k] = v;
        
        if constexpr(sizeof...(kvs))
        {
            set_zstd_opts(kvs...);
        }
    }

    expected<decompr_r> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in, size_t bufIncr = 0)
    {
        DSK_E_TRY_ONLY(init(in));

        return visit_var(_decompr, [&]<auto I>(auto& decompr) -> expected<decompr_r>
        {
            if constexpr(I != unknown) return decompr.append(d, in, bufIncr);
            else                       return {};
        });
    }

    expected<decompr_r> append_multi(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in, size_t bufIncr = 0)
    {
        DSK_E_TRY_ONLY(init(in));

        return visit_var(_decompr, [&]<auto I>(auto& decompr) -> expected<decompr_r>
        {
            if constexpr(I != unknown) return decompr.append_multi(d, in, bufIncr);
            else                       return {};
        });
    }
};


} // namespace dsk
