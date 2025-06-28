#pragma once

#include <dsk/task.hpp>
#include <dsk/asio/file.hpp>
#include <dsk/asio/transfer.hpp>
#include <boost/asio/windows/overlapped_ptr.hpp>


namespace dsk{


struct send_file_options
{
    size_t offset = 0;
    size_t count = -1; // -1 to send all remaining bytes started at offset.
};


#if defined(ASIO_HAS_WINDOWS_OVERLAPPED_PTR) || defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)


template<class Skt, class File>
struct win32_send_file_async_op
{
    Skt&                   _skt;
    File&                  _file;
    size_t                 _offset;
    DWORD                  _count;
    size_t                 _total = 0;
    error_code             _ec;
    continuation           _cont;
    optional_stop_callback _scb; // must be last one defined

    using is_async_op = void;

    bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
    {
        DSK_ASSERT(! has_err(_ec));

        if(stop_requested(ctx))
        {
            _ec = errc::canceled;
            return false;
        }

        _cont = DSK_FORWARD(cont);

        asio::windows::overlapped_ptr overlapped
        (
            _skt.get_executor(),
            [this, ctx](auto& ec, size_t n) mutable
            {
                _ec = to_ec(ec);
                _total = n;
                resume(mut_move(_cont), ctx, _skt.get_executor());
            }
        );

        auto& ov = *overlapped.get();

        ov.Offset = static_cast<DWORD>(_offset & 0xffffffff);
        ov.OffsetHigh = static_cast<DWORD>((_offset >> 32) & 0xffffffff);
        
        BOOL ok = ::TransmitFile
        (
            _skt.native_handle(),
            _file.native_handle(),
            _count,
            0,
            overlapped.get(),
            nullptr,
            0
        );
        
        DWORD err = ::GetLastError();

        if(! ok && err != ERROR_IO_PENDING)
        {
            overlapped.complete(to_ec(err), 0);
            return true; // resumed in overlapped.complete()
        }

        if(stop_possible(ctx))
        {
            _scb.emplace(get_stop_token(ctx), [this, ov]() mutable
            {
                ::CancelIoEx(_file.native_handle(), &ov);
            });
        }

        overlapped.release();
        return true;
    }

    bool is_failed() const noexcept { return has_err(_ec); }
    auto take_result() noexcept { return make_expected_if_no(_ec, _total); }

    static constexpr error_code to_ec(DWORD e) noexcept
    {
        // https://github.com/boostorg/asio/blob/6534af41b471288091ae39f9ab801594189b6fc9/include/boost/asio/detail/impl/socket_ops.ipp#L842
        switch(e)
        {
            case ERROR_NETNAME_DELETED : return asio::error::connection_reset;
            case ERROR_PORT_UNREACHABLE: return asio::error::connection_refused;
            case WSAEMSGSIZE:
            case ERROR_MORE_DATA:
                return {};
        }

        return error_code(static_cast<int>(e), system_category());
    }

    static constexpr error_code to_ec(error_code const& ec) noexcept
    {
        if(ec.category() == system_category()) return to_ec(static_cast<DWORD>(ec.value()));
        else                                   return ec;
    }
};


task<size_t> win32_send_file(auto& skt, auto& file, send_file_options opts = {})
{
    using send_op_type = win32_send_file_async_op<DSK_NO_CVREF_T(skt), DSK_NO_CVREF_T(file)>;

    if(opts.count == size_t(-1))
    {
        size_t fs = DSK_TRY_SYNC file.size();
        
        if(fs <= opts.offset)
        {
            DSK_RETURN(0);
        }

        opts.count = fs - opts.offset;
    }

    size_t left = opts.count;
    size_t offset = opts.offset;

    while(left > 0)
    {
        constexpr size_t maxCnt = std::numeric_limits<int32_t>::max() - 1;

        size_t n = DSK_TRY send_op_type(skt, file, offset, static_cast<DWORD>(std::min(left, maxCnt)));
        
        offset += n;
        left -= n;
    }

    DSK_RETURN(opts.count);
}


#endif


template<buf_size_t BufSize = buf_size_t(0)>
task<size_t> general_send_file(auto& skt, stream_file& file, send_file_options opts = {})
{
    DSK_TRY_SYNC file.seek(opts.offset, file_base::seek_set);

    DSK_TRY_RETURN(transfer<BufSize ? BufSize : buf_size_t(8*1024)>(file, skt, opts.count));
}


template<buf_size_t BufSize = buf_size_t(0)>
task<size_t> send_file(auto& skt, stream_file& file, send_file_options opts = {})
{
#if defined(ASIO_HAS_WINDOWS_OVERLAPPED_PTR) || defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
    return win32_send_file(skt, file, opts);
#else
    return general_send_file<BufSize>(skt, file, opts);
#endif
}

template<buf_size_t BufSize = buf_size_t(0)>
task<size_t> send_file(auto& skt, _borrowed_byte_str_ auto path, send_file_options opts = {})
{
    stream_file file(skt.get_executor());

    DSK_TRY_SYNC file.open_ro(path);

    DSK_TRY_RETURN(send_file<BufSize>(skt, file, opts));
}


} // namespace dsk
