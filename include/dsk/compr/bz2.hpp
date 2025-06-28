#pragma once

#include <dsk/expected.hpp>
#include <dsk/default_allocator.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/compr/common.hpp>
#include <bzlib.h>


namespace dsk{


enum class bz2_errc
{
    sequence_error   = BZ_SEQUENCE_ERROR,
    param_error      = BZ_PARAM_ERROR,
    mem_error        = BZ_MEM_ERROR,
    data_error       = BZ_DATA_ERROR,
    data_error_magic = BZ_DATA_ERROR_MAGIC,
    io_error         = BZ_IO_ERROR,
    unexpected_eof   = BZ_UNEXPECTED_EOF,
    outbuff_full     = BZ_OUTBUFF_FULL,
    config_error     = BZ_CONFIG_ERROR,    
};


class bz2_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "bz2"; }

    std::string message(int condition) const override
    {
        switch(static_cast<bz2_errc>(condition))
        {
            case bz2_errc::sequence_error   : return "sequence error";
            case bz2_errc::param_error      : return "param error";
            case bz2_errc::mem_error        : return "mem error";
            case bz2_errc::data_error       : return "data error";
            case bz2_errc::data_error_magic : return "data error magic";
            case bz2_errc::io_error         : return "io error";
            case bz2_errc::unexpected_eof   : return "unexpected eof";
            case bz2_errc::outbuff_full     : return "outbuff full";
            case bz2_errc::config_error     : return "config error";
        }

        return "undefined";
    }
};

inline constexpr bz2_err_category g_bz2_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, bz2_errc, g_bz2_err_cat)


namespace dsk{


constexpr size_t bz2_buf_inc = BZ_MAX_UNUSED;


template<auto Close, class Alloc = DSK_DEFAULT_C_ALLOC>
class bz2_base
{
protected:
    bz_stream _s;

public:
    bz2_base() noexcept
    {
        _s.state   = nullptr;
        _s.bzalloc = [](void*, int n, int m) { return Alloc::malloc(n * m); };
        _s.bzfree  = [](void*, void* d) { return Alloc::free(d); };
    }

    bz2_base(bz2_base const&) = delete;
    bz2_base& operator=(bz2_base const&) = delete;

    ~bz2_base()
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

    uint64_t total_in() const noexcept
    {
        return (uint64_t(_s.total_in_hi32) << 32) | uint64_t(_s.total_in_lo32);
    }

    uint64_t total_out() const noexcept
    {
        return (uint64_t(_s.total_out_hi32) << 32) | uint64_t(_s.total_out_lo32);
    }

    template<class T = char>
    auto* next_in() const noexcept
    {
        return alias_cast<T>(_s.next_in);
    }

    template<class T = unsigned>
    T avail_in() const noexcept
    {
        return static_cast<T>(_s.avail_in);
    }
};


struct bz2_compr_opts
{
    int blockSize100k = 9;
    int verbosity = 0;
    int workFactor = 0;
};

template<class Alloc = DSK_DEFAULT_C_ALLOC>
class bz2_compressor_t : public bz2_base<BZ2_bzCompressEnd, Alloc>
{    
public:
    bz2_errc reinit(bz2_compr_opts const& opts = {}) noexcept
    {
        this->close();

        int r = BZ2_bzCompressInit(&(this->_s), opts.blockSize100k, opts.verbosity, opts.workFactor);
        
        DSK_ASSERT(r == BZ_OK || r < 0);

        return static_cast<bz2_errc>(r);
    }

    // For BZ_FLUSH/BZ_FINISH, only first call can take a non-empty `in`.
    // Return appended size.
    template<unsigned Act = BZ_RUN>
    expected<size_t, bz2_errc> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in, size_t bufIncr = bz2_buf_inc)
    {
        if constexpr(Act == BZ_RUN)
        {
            if(buf_empty(in))
            {
                return 0;
            }
        }

        if(bufIncr < 1)
        {
            bufIncr = bz2_buf_inc;
        }

        size_t nOriD = buf_size(d);

        bz_stream& s = this->_s;

        s.next_in  = const_cast<char*>(buf_data<char>(in));
        s.avail_in = buf_size<unsigned>(in);
        
    #ifndef NDEBUG
        s.avail_out = 0;
    #endif

        for(;;)
        {
            DSK_ASSERT(s.avail_out == 0);

            auto b = buy_buf_at_least<char>(d, bufIncr);

            s.next_out  = b.data();
            s.avail_out = static_cast<unsigned>(b.size());

            int r = BZ2_bzCompress(&s, Act);

            if(r < 0)
            {
                return static_cast<bz2_errc>(r);
            };

                 if constexpr(Act == BZ_RUN   ) { if(s.avail_in == 0   ) break; }
            else if constexpr(Act == BZ_FLUSH ) { if(r == BZ_RUN_OK    ) break; }
            else if constexpr(Act == BZ_FINISH) { if(r == BZ_STREAM_END) break; }
            else
                static_assert(false, "invalid Act");
        }

        remove_buf_tail(d, s.avail_out);
        return buf_size(d) - nOriD;
    }

    // force end of current block.
    auto flush(_resizable_byte_buf_ auto& d)
    {
        return append<BZ_FLUSH>(empty_range<char>, d);
    }

    auto finish(_resizable_byte_buf_ auto& d)
    {
        return append<BZ_FINISH>(empty_range<char>, d);
    }
};

using bz2_compressor = bz2_compressor_t<>;


struct bz2_decompr_opts
{
    int verbosity = 0;
    bool small = false;
};


template<class Alloc = DSK_DEFAULT_C_ALLOC>
class bz2_decompressor_t : public bz2_base<BZ2_bzDecompressEnd, Alloc>
{    
public:
    bz2_errc reinit(bz2_decompr_opts const& opts = {}) noexcept
    {
        this->close();

        int r = BZ2_bzDecompressInit(&(this->_s), opts.verbosity, opts.small);

        DSK_ASSERT(r == BZ_OK || r < 0);

        return static_cast<bz2_errc>(r);
    }

    expected<decompr_r, bz2_errc> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                         size_t bufIncr = bz2_buf_inc)
    {
        if(buf_empty(in)) 
        {
            return decompr_r();
        }

        if(bufIncr < 1)
        {
            bufIncr = bz2_buf_inc;
        }

        bool isEnd = false;
        size_t nOriD = buf_size(d);

        bz_stream& s = this->_s;

        s.next_in  = const_cast<char*>(buf_data<char>(in));
        s.avail_in = buf_size<unsigned>(in);

    #ifndef NDEBUG
        s.avail_out = 0;
    #endif

        for(;;)
        {
            DSK_ASSERT(s.avail_out == 0);

            auto b = buy_buf_at_least<char>(d, bufIncr);

            s.next_out  = b.data();
            s.avail_out = static_cast<unsigned>(b.size());

            int r = BZ2_bzDecompress(&s);

            if(r < 0)
            {
                return static_cast<bz2_errc>(r);
            }

            if(r == BZ_STREAM_END)
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
    expected<decompr_r, bz2_errc> append_multi(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                               size_t bufIncr = bz2_buf_inc)
    {
        bool   isEnd = false;
        size_t nOriD = buf_size(d);
        auto   vin   = str_view(in);

        for(;vin.size();)
        {
            DSK_E_TRY_FWD(cur, append(d, vin, bufIncr));

            vin.remove_prefix(cur.nIn);

            if((isEnd = cur.isEnd))
            {
                DSK_E_TRY_ONLY(reinit());
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

using bz2_decompressor = bz2_decompressor_t<>;



template<class Alloc = DSK_DEFAULT_C_ALLOC>
expected<size_t, bz2_errc> bz2_compress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                               bz2_compr_opts const& opts = {}, size_t bufIncr = 0)
{
    if(bufIncr < 1)
    {
        bufIncr = static_cast<size_t>(buf_size(in) * 1.01 + 601);
    }

    bz2_compressor_t<Alloc> compr;
    DSK_E_TRY_ONLY(compr.reinit(opts));
    return compr.template append<BZ_FINISH>(d, in, bufIncr);
}

template<class Alloc = DSK_DEFAULT_C_ALLOC>
auto bz2_compress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                  bz2_compr_opts const& opts = {}, size_t bufIncr = 0)
{
    clear_buf(d);
    return bz2_compress_append<Alloc>(d, in, opts, bufIncr);
}


template<class Alloc = DSK_DEFAULT_C_ALLOC>
expected<decompr_r, bz2_errc> bz2_decompress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                    bz2_decompr_opts const& opts = {}, size_t bufIncr = bz2_buf_inc)
{
    bz2_decompressor_t<Alloc> decompr;
    DSK_E_TRY_ONLY(decompr.reinit(opts));
    return decompr.append(d, in, bufIncr);
}

template<class Alloc = DSK_DEFAULT_C_ALLOC>
auto bz2_decompress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                    bz2_decompr_opts const& opts = {}, size_t bufIncr = bz2_buf_inc)
{
    clear_buf(d);
    return bz2_decompress_append<Alloc>(d, in, opts, bufIncr);
}


} // namespace dsk
