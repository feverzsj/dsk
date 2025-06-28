#pragma once

#include <dsk/expected.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/unit.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <dsk/asio/read.hpp>
#include <dsk/asio/read_at.hpp>
#include <dsk/asio/read_until.hpp>
#include <dsk/asio/write.hpp>
#include <dsk/asio/write_at.hpp>
#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/basic_random_access_file.hpp>


namespace dsk{


using asio::file_base;


// NOTE: asio uses recycling_allocator when associated allocator is std::allocator.
//       Better not change it until proved worthy.


template<template<class Ex> class File>
class basic_file : public File<async_op_io_ctx_executor>
{
    using base = File<async_op_io_ctx_executor>;

public:
    using typename base::native_handle_type;

    using base::base;

    basic_file()
        : base(DSK_DEFAULT_IO_CONTEXT)
    {}

    basic_file(base&& f)
        : base(mut_move(f))
    {}

    basic_file(_byte_str_ auto const& path, file_base::flags f)
        : base(DSK_DEFAULT_IO_CONTEXT, cstr_data<char>(as_cstr(path)), f)
    {}

    explicit basic_file(native_handle_type const& h)
        : base(DSK_DEFAULT_IO_CONTEXT, h)
    {}

    error_code open(_byte_str_ auto const& path, file_base::flags flags)
    {
        DSK_ASIO_RETURN_EC(base::open, cstr_data<char>(as_cstr(path)), flags);
    }

    error_code open_ro (_byte_str_ auto const& path) { return open(path, file_base::read_only); }
    error_code open_wo (_byte_str_ auto const& path) { return open(path, file_base::write_only); }
    error_code open_rw (_byte_str_ auto const& path) { return open(path, file_base::read_write); }
    error_code open_rwc(_byte_str_ auto const& path) { return open(path, file_base::read_write | file_base::create); }

    error_code close()
    {
        DSK_ASIO_RETURN_EC(base::close);
    }

    error_code assign(native_handle_type const& h)
    {
        DSK_ASIO_RETURN_EC(base::assign, h);
    }

    expected<native_handle_type> release()
    {
        DSK_ASIO_RETURN_EXPECTED(base::release);
    }

    error_code cancel()
    {
        DSK_ASIO_RETURN_EC(base::cancel);
    }

    expected<uint64_t> size()
    {
        DSK_ASIO_RETURN_EXPECTED(base::size);
    }

    error_code resize(B_ bytes)
    {
        DSK_ASIO_RETURN_EC(base::resize, static_cast<uint64_t>(bytes.count()));
    }

    error_code sync_all()
    {
        DSK_ASIO_RETURN_EC(base::sync_all);
    }

    error_code sync_data()
    {
        DSK_ASIO_RETURN_EC(base::sync_data);
    }
};


class stream_file : public basic_file<asio::basic_stream_file>
{
    using base = basic_file<asio::basic_stream_file>;

public:
    using base::base;

    expected<uint64_t> seek(int64_t offset, file_base::seek_basis whence = file_base::seek_set)
    {
        DSK_ASIO_RETURN_EXPECTED(base::seek, offset, whence);
    }

    expected<uint64_t> rewind()
    {
        return seek(0, file_base::seek_set);
    }

    auto read_some(_borrowed_byte_buf_ auto&& mutableBuf)
    {
        return base::async_read_some(asio_buf(DSK_FORWARD(mutableBuf)));
    }

    auto read_some(auto const& mutableBufs)
    {
        return base::async_read_some(mutableBufs);
    }

    auto write_some(_borrowed_byte_buf_ auto&& constBuf)
    {
        return base::async_write_some(asio_buf(DSK_FORWARD(constBuf)));
    }

    auto write_some(auto const& constBufs)
    {
        return base::async_write_some(constBufs);
    }

    auto read(auto&& b, auto&&... compCond)
    {
        return dsk::read(*this, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }

    auto write(auto&& b, auto&&... compCond)
    {
        return dsk::write(*this, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }

    auto read_until(auto& b, auto&& match)
    {
        return dsk::read_until(*this, b, DSK_FORWARD(match));
    }
};


class random_access_file : public basic_file<asio::basic_random_access_file>
{
    using base = basic_file<asio::basic_random_access_file>;

public:
    using base::base;

    auto read_some_at(uint64_t offset, _borrowed_byte_buf_ auto&& mutableBuf)
    {
        return base::async_read_some_at(offset, asio_buf(DSK_FORWARD(mutableBuf)));
    }

    auto read_some_at(uint64_t offset, auto const& mutableBufs)
    {
        return base::async_read_some_at(offset, mutableBufs);
    }

    auto write_some_at(uint64_t offset, _borrowed_byte_buf_ auto&& constBuf)
    {
        return base::async_write_some_at(offset, asio_buf(DSK_FORWARD(constBuf)));
    }

    auto write_some_at(uint64_t offset, auto const& constBufs)
    {
        return base::async_write_some_at(offset, constBufs);
    }

    auto read_at(uint64_t offset, auto&& b, auto&&... compCond)
    {
        return dsk::read_at(*this, offset, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }

    auto write_at(uint64_t offset, auto&& b, auto&&... compCond)
    {
        return dsk::write_at(*this, offset, DSK_FORWARD(b), DSK_FORWARD(compCond)...);
    }
};


} // namespace dsk
