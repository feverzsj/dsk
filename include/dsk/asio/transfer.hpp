#pragma once

#include <dsk/task.hpp>
#include <dsk/asio/write.hpp>


namespace dsk{


// transfer between streams until eof.
task<size_t> transfer(auto& from, auto& to, _byte_buf_ auto& buf)
{
    for(size_t total = 0;;)
    {
        auto r = DSK_WAIT from.async_read_some(asio_buf(buf), use_async_op);
        
        if(has_err(r))
        {
            if(get_err(r) == asio::error::eof) DSK_RETURN(total);
            else                               DSK_THROW(get_err(r));
        }

        size_t n = get_val(r);

        DSK_TRY write(to, asio_buf(buf, n));

        total += n;
    }

    DSK_UNREACHABLE();
}


task<size_t> transfer(auto& from, auto& to, _byte_buf_ auto& buf, size_t count)
{
    if(count == size_t(-1))
    {
        return transfer(from, to, buf);
    }

    return [](auto& from, auto& to, _byte_buf_ auto& buf, size_t count) -> task<size_t>
    {
        size_t left = count;
        size_t bufSize = buf_size(buf);

        while(left > 0)
        {
            size_t n = DSK_TRY from.async_read_some(asio_buf(buf, std::min(left, bufSize)), use_async_op);

            DSK_TRY write(to, asio_buf(buf, n));

            left -= n;
        }

        DSK_RETURN(count);
    }(from, to, buf, count);
}


enum buf_size_t : size_t{};


template<buf_size_t BufSize>
task<size_t> transfer(auto& from, auto& to, size_t count = -1)
{
    static_assert(BufSize >= 256);

    char buf[BufSize];

    DSK_TRY_RETURN(transfer(from, to, buf, count));
}


} // namespace dsk
