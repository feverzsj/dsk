#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/compr/common.hpp>
#include <zstd.h>


namespace dsk{


class zstd_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "zstd"; }

    std::string message(int condition) const override
    {
        return ZSTD_getErrorString(static_cast<ZSTD_ErrorCode>(condition));
    }
};

inline constexpr zstd_err_category g_zstd_err_cat;


} // namespace dsk


DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(ZSTD_ErrorCode, dsk::g_zstd_err_cat)


namespace dsk{


constexpr size_t zstd_buf_inc = 5000;



inline expected<size_t> zstd_get_frame_content_size(_byte_buf_ auto const& b) noexcept
{
    auto r = ZSTD_getFrameContentSize(buf_data(b), buf_size(b));

    if(r == ZSTD_CONTENTSIZE_UNKNOWN) return errc::unknown_size;
    if(r == ZSTD_CONTENTSIZE_ERROR  ) return errc::invalid_input;
    
    return static_cast<size_t>(r);
}

inline expected<size_t, ZSTD_ErrorCode> zstd_find_frame_compressed_size(_byte_buf_ auto const& b) noexcept
{
    size_t r = ZSTD_findFrameCompressedSize(buf_data(b), buf_size(b));

    if(ZSTD_isError(r)) return ZSTD_getErrorCode(r);
    else                return r;
}

inline expected<size_t, ZSTD_ErrorCode> zstd_compress_bound(size_t n) noexcept
{
    size_t r = ZSTD_compressBound(n);

    if(ZSTD_isError(r)) return ZSTD_getErrorCode(r);
    else                return r;
}


class zstd_compr_ctx  : public unique_handle<ZSTD_CCtx*, ZSTD_freeCCtx>
{
    using base = unique_handle<ZSTD_CCtx*, ZSTD_freeCCtx>;

public:
    zstd_compr_ctx() noexcept
        : base(ZSTD_createCCtx())
    {}

    ZSTD_ErrorCode reset(ZSTD_ResetDirective m = ZSTD_reset_session_only) noexcept
    {
        return ZSTD_getErrorCode(ZSTD_CCtx_reset(this->checked_handle(), m));
    }

    ZSTD_ErrorCode set(ZSTD_cParameter k, int v) noexcept
    {
        return ZSTD_getErrorCode(ZSTD_CCtx_setParameter(this->checked_handle(), k, v));
    }

    ZSTD_ErrorCode set(ZSTD_cParameter k, int v, auto... kvs) noexcept
    {
        if(auto err = set(k, v))
        {
            return err;
        }

        return set(kvs...);
    }

    ZSTD_ErrorCode set_pledged_src_size(size_t n) noexcept
    {
        return ZSTD_getErrorCode(ZSTD_CCtx_setPledgedSrcSize(this->checked_handle(), n));
    }
};


class zstd_decompr_ctx  : public unique_handle<ZSTD_DCtx*, ZSTD_freeDCtx>
{
    using base = unique_handle<ZSTD_DCtx*, ZSTD_freeDCtx>;

public:
    zstd_decompr_ctx() noexcept
        : base(ZSTD_createDCtx())
    {}

    ZSTD_ErrorCode reset(ZSTD_ResetDirective m = ZSTD_reset_session_only) noexcept
    {
        return ZSTD_getErrorCode(ZSTD_DCtx_reset(this->checked_handle(), m));
    }

    ZSTD_ErrorCode set(ZSTD_dParameter k, int v) noexcept
    {
        return ZSTD_getErrorCode(ZSTD_DCtx_setParameter(this->checked_handle(), k, v));
    }

    ZSTD_ErrorCode set(ZSTD_dParameter k, int v, auto... kvs) noexcept
    {
        if(auto err = set(k, v))
        {
            return err;
        }

        return set(kvs...);
    }
};


class zstd_compressor  : public zstd_compr_ctx
{
public:
    // Append 'in' as a whole frame.
    // Return appended size.
    expected<size_t, ZSTD_ErrorCode> append_frame(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in)
    {
        DSK_E_TRY(size_t n, zstd_compress_bound(buf_size(in)));

        auto r = ZSTD_compress2(checked_handle(), buy_buf(d, n), n, buf_data(in), buf_size(in));

        if(ZSTD_isError(r))
        {
            return ZSTD_getErrorCode(r);
        }

        remove_buf_tail(d, n - r);
        return r;
    }

    // For ZSTD_e_end/flush, only first call can take a non-empty `in`.
    // Return appended size.
    template<ZSTD_EndDirective Act = ZSTD_e_continue>
    expected<size_t, ZSTD_ErrorCode> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                            size_t bufIncr = zstd_buf_inc)
    {
        if constexpr(Act == ZSTD_e_continue)
        {
            if(buf_empty(in))
            {
                return 0;
            }
        }

        if(bufIncr < 1)
        {
            bufIncr = zstd_buf_inc;
        }

        size_t nOriD = buf_size(d);

        ZSTD_inBuffer ib;
        ib.src  = buf_data(in);
        ib.size = buf_size<uInt>(in);
        ib.pos  = 0;
        
        ZSTD_outBuffer ob;
        ob.size = 0;
        ob.pos  = 0;

        for(;;)
        {
            if(ob.pos >= ob.size)
            {
                auto b = buy_buf_at_least(d, bufIncr);

                ob.dst  = b.data();
                ob.size = b.size();
                ob.pos  = 0;
            }

            size_t r = ZSTD_compressStream2(checked_handle(), &ob, &ib, Act);

            if(ZSTD_isError(r))
            {
                return ZSTD_getErrorCode(r);
            };

            if constexpr(Act == ZSTD_e_continue) { if(ib.pos >= ib.size) break; }
            else                                 { if(r == 0) break; }

            if(r > bufIncr)
            {
                bufIncr = r;
            }
        }

        remove_buf_tail(d, ob.size - ob.pos);
        return buf_size(d) - nOriD;
    }

    // force end of current block.
    auto flush(_resizable_byte_buf_ auto& d)
    {
        return append<ZSTD_e_flush>(d, empty_range<Bytef>);
    }

    auto finish(_resizable_byte_buf_ auto& d)
    {
        return append<ZSTD_e_end>(d, empty_range<Bytef>);
    }
};


class zstd_decompressor : public zstd_decompr_ctx
{    
public:
    // Decompress the first frame of 'in'.
    // 'in' must contain a whole frame.
    // Return appended size.
    expected<size_t> append_frame(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in)
    {
        DSK_E_TRY(size_t n, zstd_get_frame_content_size(in));

        if(n == 0) // empty content, e.g.: skippable frame
        {
            return 0;
        }

        size_t r = ZSTD_decompressDCtx(checked_handle(), buy_buf(d, n), n, buf_data(in), buf_size(in));

        if(ZSTD_isError(r))
        {
            return ZSTD_getErrorCode(r);
        }

        remove_buf_tail(d, n - r);
        return r;
    }

    expected<decompr_r, ZSTD_ErrorCode> append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                               size_t bufIncr = zstd_buf_inc)
    {
        if(buf_empty(in)) 
        {
            return decompr_r();
        }

        if(bufIncr < 1)
        {
            bufIncr = zstd_buf_inc;
        }

        bool isEnd = false;
        size_t nOriD = buf_size(d);

        ZSTD_inBuffer ib;
        ib.src  = buf_data(in);
        ib.size = buf_size(in);
        ib.pos  = 0;

        ZSTD_outBuffer ob;
        ob.size = 0;
        ob.pos  = 0;

        for(;;)
        {
            if(ob.pos >= ob.size)
            {
                auto b = buy_buf_at_least(d, bufIncr);

                ob.dst  = b.data();
                ob.size = b.size();
                ob.pos  = 0;
            }

            size_t r = ZSTD_decompressStream(checked_handle(), &ob, &ib);

            if(ZSTD_isError(r))
            {
                return ZSTD_getErrorCode(r);
            };

            if(r == 0)
            {
                isEnd = true;
                break;
            }

            if(ib.pos >= ib.size)
            {
                break;
            }
        }

        remove_buf_tail(d, ob.size - ob.pos);

        return decompr_r
        {
            .nIn = ib.pos,
            .nOut = buf_size(d) - nOriD,
            .isEnd = isEnd
        };
    }

    // append concatenated frames(streams)
    expected<decompr_r, ZSTD_ErrorCode> append_multi(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                     size_t bufIncr = zstd_buf_inc)
    {
        bool   isEnd = false;
        size_t nOriD = buf_size(d);
        auto   vin   = str_view(in);

        for(;vin.size();)
        {
            DSK_E_TRY_FWD(cur, append(d, vin, bufIncr));

            vin.remove_prefix(cur.nIn);

            isEnd = cur.isEnd;
            // zstd automatically reset after a frame is finished
            //if(cur.isEnd)
            //{
            //    DSK_E_TRY_ONLY(reset());
            //}
        }

        return decompr_r
        {
            .nIn = buf_size(in) - vin.size(),
            .nOut = buf_size(d) - nOriD,
            .isEnd = isEnd
        };
    }
};


expected<size_t, ZSTD_ErrorCode> zstd_compress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                      auto... params)
{
    zstd_compressor compr;

    if constexpr(sizeof...(params))
    {
        DSK_E_TRY_ONLY(compr.set(params...));
    }

    return compr.append_frame(d, in);
}

auto zstd_compress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in, auto... params)
{
    clear_buf(d);
    return zstd_compress_append(d, in, params...);
}


expected<decompr_r, ZSTD_ErrorCode> zstd_decompress_append(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                                                           size_t bufIncr = zstd_buf_inc, auto... params)
{
    zstd_decompressor decompr;

    if constexpr(sizeof...(params))
    {
        DSK_E_TRY_ONLY(decompr.set(params...));
    }

    return decompr.append(d, in, bufIncr);
}

auto zstd_decompress(_resizable_byte_buf_ auto& d, _byte_buf_ auto const& in,
                     size_t bufIncr = zstd_buf_inc, auto... params)
{
    clear_buf(d);
    return zstd_decompress_append(d, in, bufIncr, params...);
}


} // namespace dsk
