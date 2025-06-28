#pragma once

#include <dsk/optional.hpp>
#include <dsk/charset/ascii.hpp>
#include <dsk/util/from_str.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/sqlite3/conn.hpp>
#include <dsk/sqlite3/stmt.hpp>


namespace dsk{


// NOTE: recommand to destory or explicitly close() this object before closing conn,
//       even though sqlite3_close_v2() will close db when last prepared statement get destroyed.
class sqlite3_settings_tbl
{
    sqlite3_pstmt _get;
    sqlite3_pstmt _set;

public:
    sqlite3_settings_tbl() = default;

    error_code init(sqlite3_conn& conn, _byte_str_ auto const& tbl)
    {
        DSK_E_TRY_ONLY(conn.exec(cat_str(
            "CREATE TABLE IF NOT EXISTS ", tbl, "("
            "name TEXT PRIMARY KEY NOT NULL,"
            "value BLOB)"
        )));

        DSK_E_TRY(_get, conn.prepare(cat_str("SELECT value FROM ", tbl, " WHERE name==?")));
        DSK_E_TRY(_set, conn.prepare(cat_str("INSERT INTO ", tbl, "(name, value) VALUES(?,?) "
                                             "ON CONFLICT(name) DO UPDATE SET value = excluded.value")));
        return {};
    }

    void close()
    {
        _get.close();
        _set.close();
    }

    expected<> set(_byte_str_ auto const& name, _stringifible_  auto&& v)
    {
        return _set.exec(name, stringify(DSK_FORWARD(v)));
    }

    expected<> set(auto const& k, auto const& v, auto const&... kvs)
    {
        DSK_E_TRY_ONLY(set(k, v));
        return set(kvs...);
    }

    struct scoped_getter_t
    {
        sqlite3_pstmt& _get;

        explicit scoped_getter_t(sqlite3_pstmt& g) noexcept : _get{g} {}

        ~scoped_getter_t() { _get.reset_state(); }

        expected<bool> get(_byte_str_ auto const& name, auto& v)
        {
            DSK_E_TRY_ONLY(_get.arg(1, name));

            if constexpr(_buf_<decltype(v)>)
            {
                return _get.next(v);
            }
            else
            {
                DSK_E_TRY_FWD(sv, _get.next<std::optional<std::string_view>>());

                if(! sv)
                {
                    return false;
                }

                DSK_E_TRY_ONLY(from_whole_str(*sv, v));
                return true;
            }
        }

        expected<bool> get(_byte_str_ auto const& name, auto& v, auto&& def)
        {
            DSK_E_TRY(bool found, get(name, v));

            if(found)
            {
                return true;
            }

            v = DSK_FORWARD(def);
            return false;
        }

        template<class T>
        expected<std::optional<T>> value(_byte_str_ auto const& name)
        {
            T v;

            DSK_E_TRY(bool found, get(name, v));

            if(found) return v;
            else      return nullopt;
        }

        template<class T = void, bool UseSv = true> // if T = void, auto deduce return type
        auto value(_byte_str_ auto const& name, auto&& def)
        {
            constexpr auto find_ret_type = []()
            {
                if constexpr(_void_<T>)
                {
                    using def_t = DSK_NO_CVREF_T(def);

                    if constexpr(_str_<def_t> && ! _class_<def_t>)
                    {
                        if constexpr(UseSv) return std::basic_string_view<str_val_t<def_t>>();
                        else                return basic_string<str_val_t<def_t>>();
                    }
                    else
                    {
                        return def_t();
                    }
                }
                else
                {
                    return T();
                }
            };

            using ret_t = decltype(find_ret_type());

            ret_t v;

            DSK_E_TRY(bool found, get(name, v));

            if(found) return expected<ret_t>(expect, mut_move(v));
            else      return expected<ret_t>(expect, DSK_FORWARD(def));
        }
    };

    // non-ownership values like string_view are valid within the scoped_getter's scope
    scoped_getter_t scoped_getter() noexcept { return scoped_getter_t{_get}; }
    
    // NOTE: if value is non-ownership, use scoped_getter instead
    expected<bool> get(_byte_str_ auto const& name,auto& v)
    {
        auto getter = scoped_getter();
        return getter.get(name, v);
    }

    expected<bool> get(_byte_str_ auto const& name, auto& v, auto&& def)
    {
        auto getter = scoped_getter();
        return getter.get(name, v, DSK_FORWARD(def));
    }

    template<class T>
    expected<optional<T>> value(_byte_str_ auto const& name)
    {
        auto getter = scoped_getter();
        return getter.template value<T>(name);
    }

    template<class T = void> // if T = void, auto deduce return type
    auto value(_byte_str_ auto const& name, auto&& def)
    {
        auto getter = scoped_getter();
        return getter.template value<T, false>(name, DSK_FORWARD(def));
    }
};


class sqlite3_settings_db : public sqlite3_settings_tbl
{
    sqlite3_conn _conn;

public:
    sqlite3_settings_db(sqlite3_settings_db&&) = default;
    ~sqlite3_settings_db() { close(); }

    sqlite3_conn& conn() noexcept { return _conn; }

    error_code open(_byte_str_ auto const& path, _byte_str_ auto const& tbl)
    {
        DSK_E_TRY_ONLY(_conn.open(path));
        DSK_E_TRY_ONLY(_conn.set_journal_mode(journal_mode_wal));
        DSK_E_TRY_ONLY(_conn.set_synchronous_mode(synchronous_mode_normal));

        return sqlite3_settings_tbl::init(_conn, tbl);
    }

    void close()
    {
        sqlite3_settings_tbl::close();
        _conn.close();
    }
};


} // namespace dsk
