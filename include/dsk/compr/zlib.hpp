#pragma once

#include <dsk/expected.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/compr/common.hpp>
#include <zlib.h>


namespace dsk{


enum class [[nodiscard]] zlib_errc
{
    errno_error   = Z_ERRNO,
    stream_error  = Z_STREAM_ERROR,
    data_error    = Z_DATA_ERROR,
    mem_error     = Z_MEM_ERROR,
    buf_error     = Z_BUF_ERROR,
    version_error = Z_VERSION_ERROR,
    need_dict     = Z_NEED_DICT // not < 0
};


class zlib_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "zlib"; }

    std::string message(int condition) const override
    {
        switch(static_cast<zlib_errc>(condition))
        {
            case zlib_errc::errno_error   : return "errno error";
            case zlib_errc::stream_error  : return "stream error";
            case zlib_errc::data_error    : return "data error";
            case zlib_errc::mem_error     : return "memory error";
            case zlib_errc::buf_error     : return "buffer error";
            case zlib_errc::version_error : return "version error";
            case zlib_errc::need_dict     : return "need dict";
        }

        return "undefined";
    }
};

inline constexpr zlib_err_category g_zlib_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, zlib_errc, g_zlib_err_cat)


namespace dsk{


constexpr size_t zlib_buf_inc = 5000;


template<auto Close, class Alloc = DSK_DEFAULT_C_ALLOC>
class zlib_base
{
protected:
    z_stream _s;

public:
    zlib_base() noexcept
    {
        _s.state   = nullptr;
        _s.zalloc = [](void*, uInt n, uInt m) { return Alloc::malloc(n * m); };
        _s.zfree  = [](void*, void* d) { return Alloc::free(d); };
    }

    zlib_base(zlib_base const&) = delete;
    zlib_base& operator=(zlib_base const&) = delete;

    ~zlib_base()
    {
        close();
    }

    void close() noexcept
    {
        if(valid())
        {
            Close(&_s);
            DSK_ASSERT(! valid());
        }
    }

    bool valid() const noexcept
    {
        return _s.state != nullptr;
    }

    explicit operator bool() const noexcept
    {
        return valid();
    }

    uLong total_in() const noexcept
    {
        return _s.total_in;
    }

    uLong total_out() const noexcept
    {
        return _s.total_out;
    }

    template<class T = Bytef>
    auto* next_in() const noexcept
    {
        return alias_cast<T>(_s.next_in);
    }

    template<class T = uInt>
    T avail_in() const noexcept
    {
        return static_cast<T>(_s.avail_in);
    }
};


struct zlib_compr_opts
{
    bool gzip       = false;
    int  windowBits = MAX_WBITS;
    int  level      = Z_DEFAULT_COMPRESSION;
    int  memLevel   = 8;
    int  strategy   = Z_DEFAULT_STRATEGY;

    constexpr int real_window_bits() const noexcept
    {
        DSK_ASSERT(! gzip || windowBits > 0);

        return (gzip ? (windowBits + 16) : windowBits);
    }
};

template<class Alloc = DSK_DEFAULT_C_ALLOC>
class zlib_compressor_t : public zlib_base<deflateEnd, Alloc>
{    
public:
    zlib_errc reinit(zlib_compr_opts const& opts = {}) noexcept
    {
        this->close();

        int r = deflateInit2(&(this->_s), opts.level,
                                          Z_DEFLATED,
                                          opts.real_window_bits(),
                                          opts.memLevel,
                                          opts.strategy);
        
        DSK_ASSERT(r == Z_OK || r < 0);

        return static_cast<zlib_errc>(r);
    }

    zlib_errc reset() noexcept
    {
        return static_cast<zlib_errc>(deflateReset(&(this->_s)));
    }

    // 'h' should exist until compress finish.
    zlib_errc set_gzip_hdr(gz_header& h) noexcept
    {
        return static_cast<zlib_errc>(deflateSetHeader(&(this->_s), &h));
    }

    size_t compress_bound(size_t n) noexcept
    {
        DSK_ASSERT(this->valid());
        return static_cast<size_t>(deflateBound(&(this->_s), static_cast<uLong>(n)));
    }

    // For Z_FINISH/flush, only first call can take a non-empty `in`.
    // Return appended size.
    template<int Act = Z_NO_FLUSH>
    expected<size_t, zlib_errc> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                       size_t bufIncr = zlib_buf_inc)
    {
        if constexpr(Act == Z_NO_FLUSH)
        {
            if(buf_empty(in))
            {
                return 0;
            }
        }

        if(bufIncr < 1)
        {
            bufIncr = zlib_buf_inc;
        }

        size_t nOriD = buf_size(d);

        z_stream& s = this->_s;

        s.next_in  = const_cast<Bytef*>(buf_data<Bytef>(in));
        s.avail_in = buf_size<uInt>(in);
        
    #ifndef NDEBUG
        s.avail_out = 0;
    #endif

        for(;;)
        {
            DSK_ASSERT(s.avail_out == 0);

            auto b = buy_buf_at_least<Bytef>(d, bufIncr);

            s.next_out  = b.data();
            s.avail_out = static_cast<uInt>(b.size());

            int r = deflate(&s, Act);

            if(r == Z_BUF_ERROR)
            {
                continue;
            }

            if(r < 0)
            {
                return static_cast<zlib_errc>(r);
            };

                 if constexpr(Act == Z_NO_FLUSH) { if(s.avail_in == 0  ) break; }
            else if constexpr(Act == Z_FINISH  ) { if(r == Z_STREAM_END) break; }
            else              /* flush modes */  { if(s.avail_out > 0  ) break; }
        }

        remove_buf_tail(d, s.avail_out);
        return buf_size(d) - nOriD;
    }

    // force end of current block.
    template<int Act = Z_SYNC_FLUSH>
    auto flush(_resizable_byte_buf_ auto& d)
    {
        return append<Act>(empty_range<Bytef>, d);
    }

    auto finish(_resizable_byte_buf_ auto& d)
    {
        return append<Z_FINISH>(empty_range<Bytef>, d);
    }
};

using zlib_compressor = zlib_compressor_t<>;


struct zlib_decompr_opts
{
    bool gzip       = false;
    int  windowBits = INT_MIN;

    constexpr int real_window_bits() const noexcept
    {
        if(windowBits == INT_MIN)
        {
            return MAX_WBITS + (gzip ? 16 : 32); // +16: gzip, +32: auto detect
        }

        DSK_ASSERT(! gzip || windowBits > 0);
        return (gzip ? (windowBits + 16) : windowBits);
    }
};


template<class Alloc = DSK_DEFAULT_C_ALLOC>
class zlib_decompressor_t : public zlib_base<inflateEnd, Alloc>
{    
public:
    zlib_errc reinit(zlib_decompr_opts const& opts = {}) noexcept
    {
        this->close();

        int r = inflateInit2(&(this->_s), opts.real_window_bits());

        DSK_ASSERT(r == Z_OK || r < 0);

        return static_cast<zlib_errc>(r);
    }

    zlib_errc reset() noexcept
    {
        return static_cast<zlib_errc>(inflateReset(&(this->_s)));
    }

    zlib_errc reset(zlib_decompr_opts const& opts) noexcept
    {
        return static_cast<zlib_errc>(inflateReset2(&(this->_s), opts.real_window_bits()));
    }

    zlib_errc sync() noexcept
    {
        return static_cast<zlib_errc>(inflateSync(&(this->_s)));
    }

    // 'h' should exist until header has been read(when h.done != 0).
    zlib_errc set_gzip_hdr_for_read(gz_header& h) noexcept
    {
        return static_cast<zlib_errc>(inflateGetHeader(&(this->_s), &(h)));
    }

    zlib_errc set_dict(_byte_buf_ auto const& b) noexcept
    {
        return static_cast<zlib_errc>(inflateSetDictionary(&(this->_s), buf_data<Bytef>(b), buf_size<uInt>(b)));
    }

    zlib_errc get_dict(_resizable_byte_buf_ auto& b)
    {
        resize_buf<minimum_growth>(b, 32768);
        uInt n = 0;

        int r = inflateGetDictionary(&(this->_s), buf_data<Bytef>(b), &n);

        if(r < 0)
        {
            return static_cast<zlib_errc>(r);
        }

        resize_buf(b, n);
        return {};
    }

    template<int Act = Z_NO_FLUSH>
    expected<decompr_r, zlib_errc> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                          size_t bufIncr = zlib_buf_inc)
    {
        if(buf_empty(in)) 
        {
            return decompr_r();
        }

        if(bufIncr < 1)
        {
            bufIncr = zlib_buf_inc;
        }

        bool isEnd = false;
        size_t nOriD = buf_size(d);

        z_stream& s = this->_s;

        s.next_in  = const_cast<Bytef*>(buf_data<Bytef>(in));
        s.avail_in = buf_size<uInt>(in);

    #ifndef NDEBUG
        s.avail_out = 0;
    #endif

        for(;;)
        {
            DSK_ASSERT(s.avail_out == 0);

            auto b = buy_buf_at_least<Bytef>(d, bufIncr);

            s.next_out  = b.data();
            s.avail_out = static_cast<uInt>(b.size());

            int r = inflate(&s, Act);

            if(r == Z_BUF_ERROR)
            {
                continue;
            }

            if(r < 0 || r == Z_NEED_DICT)
            {
                return static_cast<zlib_errc>(r);
            }

            if(r == Z_STREAM_END)
            {
                isEnd = true;
                break;
            }

            if(s.avail_in == 0)
            {
                break;
            }
        }

        remove_buf_tail(d, s.avail_out);

        return decompr_r
        {
            .nIn = buf_size(in) - s.avail_in,
            .nOut = buf_size(d) - nOriD,
            .isEnd = isEnd
        };
    }

    // append concatenated streams
    template<int Act = Z_NO_FLUSH>
    expected<decompr_r, zlib_errc> append_multi(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                size_t bufIncr = zlib_buf_inc)
    {
        bool   isEnd = false;
        size_t nOriD = buf_size(d);
        auto   vin   = str_view(in);

        for(;vin.size();)
        {
            DSK_E_TRY_FWD(cur, append<Act>(d, vin, bufIncr));

            vin.remove_prefix(cur.nIn);

            if((isEnd = cur.isEnd))
            {
                DSK_E_TRY_ONLY(reset());
            }
        }

        return decompr_r
        {
            .nIn = buf_size(in) - vin.size(),
            .nOut = buf_size(d) - nOriD,
            .isEnd = isEnd
        };
    }
};

using zlib_decompressor = zlib_decompressor_t<>;



template<class Alloc = DSK_DEFAULT_C_ALLOC>
expected<size_t, zlib_errc> zlib_compress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                 zlib_compr_opts const& opts = {}, size_t bufIncr = 0)
{
    zlib_compressor_t<Alloc> compr;
    DSK_E_TRY_ONLY(compr.reinit(opts));

    if(bufIncr < 1)
    {
        bufIncr = compr.compress_bound(buf_size(in));
    }

    return compr.template append<Z_FINISH>(d, in, bufIncr);
}

template<class Alloc = DSK_DEFAULT_C_ALLOC>
auto zlib_compress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                   zlib_compr_opts const& opts = {}, size_t bufIncr = 0)
{
    clear_buf(d);
    return zlib_compress_append<Alloc>(d, in, opts, bufIncr);
}


template<class Alloc = DSK_DEFAULT_C_ALLOC>
expected<decompr_r, zlib_errc> zlib_decompress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                      zlib_decompr_opts const& opts = {}, size_t bufIncr = zlib_buf_inc)
{
    zlib_decompressor_t<Alloc> decompr;
    DSK_E_TRY_ONLY(decompr.reinit(opts));
    return decompr.template append<Z_FINISH>(d, in, bufIncr);
}

template<class Alloc = DSK_DEFAULT_C_ALLOC>
auto zlib_decompress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                     zlib_decompr_opts const& opts = {}, size_t bufIncr = zlib_buf_inc)
{
    clear_buf(d);
    return zlib_decompress_append<Alloc>(d, in, opts, bufIncr);
}


} // namespace dsk
