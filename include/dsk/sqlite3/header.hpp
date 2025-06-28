#pragma once

#include <dsk/util/buf.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/endian.hpp>
#include <boost/integer.hpp>


namespace dsk::sqlite3_header{


// https://www.sqlite.org/fileformat.html


inline constexpr size_t hdr_size = 100;


template<size_t Offset, size_t N>
struct fixed_buf_t
{
    static std::array<char, N> read(uint8_t const* b) noexcept
    {
        std::array<char, N> s;
        memcpy(s.data(), b + Offset, N);
        return s;
    }
};

template<size_t Offset, size_t N>
struct uint_t
{
    static auto read(uint8_t const* b) noexcept
    {
        return load_be<typename boost::uint_t<N * 8>::least, N>(b + Offset);
    }
};

using hdrStr                 = fixed_buf_t<0, 16>; // "SQLite format 3\0"
using pageSize               = uint_t<16, 2>;
using writeVer               = uint_t<18, 1>; // 1 for legacy; 2 for WAL.
using readVer                = uint_t<19, 1>; // 1 for legacy; 2 for WAL.
using extraPageSpace         = uint_t<20, 1>; // usually 0
using maxEmbeddedPayloadFrac = uint_t<21, 1>; // 64
using minEmbeddedPayloadFrac = uint_t<22, 1>; // 32
using leafPayloadFrac        = uint_t<23, 1>; // 32
using fileChangeCnt          = uint_t<24, 4>;
using pageCnt                = uint_t<28, 4>;
using firstFreelistTrunkPage = uint_t<32, 4>;
using freelistPageCnt        = uint_t<36, 4>;
using schemaCookie           = uint_t<40, 4>;
using schemaForamt           = uint_t<44, 4>;
using defaultPageCacheSize   = uint_t<48, 4>;
using largestRootBtreePage   = uint_t<52, 4>;
using textEncoding           = uint_t<56, 4>; // 1=UTF-8, 2=UTF-16le, 3=UTF-16be
using usrVer                 = uint_t<60, 4>; // PRAGMA user_version
using incVacuum              = uint_t<64, 4>;
using appId                  = uint_t<68, 4>; // PRAGMA application_id
using verValidFor            = uint_t<92, 4>; // fileChangeCnt when sqliteVer is stored
using sqliteVer              = uint_t<96, 4>; // the SQLITE_VERSION_NUMBER of sqlite lib that most recently modified the database file


template<class... Fld>
constexpr auto read(_byte_buf_ auto const& b) noexcept
{
    static_assert(sizeof...(Fld));
    DSK_ASSERT(buf_size(b) >= hdr_size);

    uint8_t const* d = buf_data<uint8_t>(b);

    return tuple(Fld::read(d)...);
}


} // namespace dsk::sqlite3_header
