#pragma once

#include <dsk/sqlite3/conn.hpp>

#include <spatialite.h>


namespace dsk{


class spatialite_conn : public sqlite3_conn
{
    unique_handle<void*, spatialite_cleanup_ex> _ctx;

public:
    error_code open(_byte_str_ auto const& path, sqlite3_flags_t flags = sqlite3_flags.rwc(), char const* vfs = nullptr)
    {
        decltype(_ctx) ctx(spatialite_alloc_connection());
        
        if(! ctx)
        {
            return sqlite3_errc::nomem;
        }

        DSK_E_TRY_ONLY(sqlite3_conn::open(path, flags, vfs));
        _ctx = mut_move(ctx);

        spatialite_init_ex(checked_handle(), _ctx.get(), 0);
        return {};
    }

    void close()
    {
        _ctx.reset();
        sqlite3_conn::close();
    }

    // http://www.gaia-gis.it/gaia-sins/spatialite-sql-latest.html#p16
    // 
    // srs: spatial reference system, only corresponding ESPG SRID definition will be inserted into the spatial_ref_sys table
    //      WGS84/WGS84_ONLY
    //      NONE/EMPTY : no ESPG SRID definition will be inserted into the spatial_ref_sys table
    //      
    // NOTE: if meta table already exits, this method will do nothing and return false,
    //       even if the 'srs' is different.
    expected<bool> init_spatial_meta_data(_byte_str_ auto const& srs,bool singleTransaction = true)
    {
        return exec<bool>("SELECT InitSpatialMetaData(?, ?)", singleTransaction, srs);
    }

    // all possible ESPG SRID definitions will be inserted into the spatial_ref_sys table
    expected<bool> init_spatial_meta_data(bool singleTransaction = true)
    {
        return exec<bool>("SELECT InitSpatialMetaData(?)", singleTransaction);
    }
};


} // namespace std
