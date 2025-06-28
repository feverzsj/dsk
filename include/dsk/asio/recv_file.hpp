#pragma once

#include <dsk/task.hpp>
#include <dsk/asio/file.hpp>
#include <dsk/asio/transfer.hpp>


namespace dsk{


struct recv_file_options
{
    size_t offset = -1; // -1 to write to current position.
    size_t count  = -1; // -1 to receive all remaining bytes until eof.
    file_base::flags flags = file_base::append | file_base::create;
};


template<buf_size_t BufSize = buf_size_t(0)>
task<size_t> recv_file(auto& skt, stream_file& file, recv_file_options opts = {})
{    
    if(opts.offset != size_t(-1))
    {
        DSK_TRY_SYNC file.seek(opts.offset, file_base::seek_set);
    }

    DSK_TRY_RETURN(transfer<BufSize ? BufSize : buf_size_t(8*1024)>(skt, file, buf, opts.count));
}


template<buf_size_t BufSize = buf_size_t(0)>
task<size_t> recv_file(auto& skt, _borrowed_byte_str_ auto path, recv_file_options opts = {})
{
    stream_file file(skt.get_executor());
    DSK_TRY_SYNC file.open(path, file_base::write_only | opts.flags);
    DSK_TRY_RETURN(recv_file<BufSize>(skt, file, opts));
}


} // namespace dsk
