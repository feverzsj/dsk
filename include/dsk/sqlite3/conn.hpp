#pragma once

#include <dsk/sqlite3/err.hpp>
#include <dsk/sqlite3/stmt.hpp>
#include <dsk/async_op.hpp>
#include <dsk/charset/ascii.hpp>
#include <dsk/util/unit.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <compare>


namespace dsk{


enum auto_vacuum_mode_e
{
    auto_vacuum_none = 0,
    auto_vacuum_full = 1,
    auto_vacuum_inc  = 2
};

enum locking_mode_e
{
    locking_mode_normal,
    locking_mode_exclusive
};

enum journal_mode_e
{
    journal_mode_delete,
    journal_mode_truncate,
    journal_mode_persist,
    journal_mode_memory,
    journal_mode_wal,
    journal_mode_off
};

enum synchronous_mode_e
{
    synchronous_mode_off    = 0,
    synchronous_mode_normal = 1,
    synchronous_mode_full   = 2,
    synchronous_mode_extra  = 3
};

enum temp_store_e
{
    temp_store_default = 0,
    temp_store_file    = 1,
    temp_store_memory  = 2
};

enum transaction_e
{
    transaction_deferred,
    transaction_immediate,
    transaction_exclusive
};


constexpr char const* locking_mode_to_str(locking_mode_e m) noexcept
{
    switch(m)
    {
        case locking_mode_normal   : return "NORMAL";
        case locking_mode_exclusive: return "EXCLUSIVE";
    }

    DSK_ASSERT(false);
}

constexpr char const* journal_mode_to_str(journal_mode_e m) noexcept
{
    switch(m)
    {
        case journal_mode_delete  : return "DELETE";
        case journal_mode_truncate: return "TRUNCATE";
        case journal_mode_persist : return "PERSIST";
        case journal_mode_memory  : return "MEMORY";
        case journal_mode_wal     : return "WAL";
        case journal_mode_off     : return "OFF";
    }

    DSK_ASSERT(false);
}

constexpr char const* transaction_to_str(transaction_e m) noexcept
{
    switch(m)
    {
        case transaction_deferred : return "DEFERRED";
        case transaction_immediate: return "IMMEDIATE";
        case transaction_exclusive: return "EXCLUSIVE";
    }

    DSK_ASSERT(false);
}


class sqlite3_flags_t
{
    explicit constexpr sqlite3_flags_t(int v) : value(v) {}

public:
    int const value = 0;

    constexpr sqlite3_flags_t() = default;

    constexpr auto operator<=>(sqlite3_flags_t const&) const = default;
    constexpr auto operator|(sqlite3_flags_t r) { return sqlite3_flags_t(value | r.value); }

    // flag clear
    // if uri/mutex/cache flags are cleared, method like sqlite3_conn.open may use sqlite3_global setting, or attach could use conn's openFlags.
    [[nodiscard]] constexpr auto clear          () const { return sqlite3_flags_t(0); }
    [[nodiscard]] constexpr auto clear_rwc      () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_READONLY | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)); }
    [[nodiscard]] constexpr auto clear_uri      () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_URI)); }
    [[nodiscard]] constexpr auto clear_mutex    () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_NOMUTEX)); }
    [[nodiscard]] constexpr auto clear_cache    () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_SHAREDCACHE | SQLITE_OPEN_PRIVATECACHE)); }
    [[nodiscard]] constexpr auto clear_nofollow () const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_NOFOLLOW)); }
    [[nodiscard]] constexpr auto clear_exrescode() const { return sqlite3_flags_t(value & ~(SQLITE_OPEN_EXRESCODE)); }
    [[nodiscard]] constexpr auto clear_non_rwcm () const { return clear_uri().clear_mutex().clear_cache(); }

    // flag set
    [[nodiscard]] constexpr auto ro () const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READONLY                      ); }
    [[nodiscard]] constexpr auto rw () const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READWRITE                     ); }
    [[nodiscard]] constexpr auto rwc() const { return clear_rwc() | sqlite3_flags_t(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE); }
    [[nodiscard]] constexpr auto mem() const { return       rwc() | sqlite3_flags_t(SQLITE_OPEN_MEMORY                        ); }

    [[nodiscard]] constexpr auto uri         () const { return clear_uri      () | sqlite3_flags_t(SQLITE_OPEN_URI         ); }
    [[nodiscard]] constexpr auto fullmutex   () const { return clear_mutex    () | sqlite3_flags_t(SQLITE_OPEN_FULLMUTEX   ); }
    [[nodiscard]] constexpr auto nomutex     () const { return clear_mutex    () | sqlite3_flags_t(SQLITE_OPEN_NOMUTEX     ); }
    [[nodiscard]] constexpr auto sharedcache () const { return clear_cache    () | sqlite3_flags_t(SQLITE_OPEN_SHAREDCACHE ); }
    [[nodiscard]] constexpr auto privatecache() const { return clear_cache    () | sqlite3_flags_t(SQLITE_OPEN_PRIVATECACHE); }
    [[nodiscard]] constexpr auto nofollow    () const { return clear_nofollow () | sqlite3_flags_t(SQLITE_OPEN_NOFOLLOW    ); }
    [[nodiscard]] constexpr auto exrescode   () const { return clear_exrescode() | sqlite3_flags_t(SQLITE_OPEN_EXRESCODE   ); }

    // flag test
    // NOTE: is_rw() include is_rwc(), so one should test is_rwc() first
    constexpr bool is_empty       () const { return  value == 0                                  ; }
    constexpr bool is_ro          () const { return  value & SQLITE_OPEN_READONLY                ; }
    constexpr bool is_rw          () const { return  value & SQLITE_OPEN_READWRITE               ; }
    constexpr bool is_rwc         () const { return (value & SQLITE_OPEN_CREATE      ) && is_rw(); }
    constexpr bool is_mem         () const { return  value & SQLITE_OPEN_MEMORY                  ; }
    constexpr bool is_uri         () const { return  value & SQLITE_OPEN_URI                     ; }
    constexpr bool is_nomutex     () const { return  value & SQLITE_OPEN_NOMUTEX                 ; }
    constexpr bool is_fullmutex   () const { return  value & SQLITE_OPEN_FULLMUTEX               ; }
    constexpr bool is_sharedcache () const { return  value & SQLITE_OPEN_SHAREDCACHE             ; }
    constexpr bool is_privatecache() const { return  value & SQLITE_OPEN_PRIVATECACHE            ; }
    constexpr bool is_nofollow    () const { return  value & SQLITE_OPEN_NOFOLLOW                ; }
    constexpr bool is_exrescode   () const { return  value & SQLITE_OPEN_EXRESCODE               ; }

    template<_resizable_byte_buf_ D = string>
    [[nodiscard]] D to_uri_query(char const* vfs = nullptr) const
    {
        D d;

        if(is_empty())
        {
            return d;
        }

        char const* mode = nullptr;
             if(is_ro ()) mode = "ro";
        else if(is_rwc()) mode = "rwc";
        else if(is_rw ()) mode = "rw";
        else if(is_mem()) mode = "memory";

        char const* cache = nullptr;
             if(is_sharedcache ()) cache = "shared";
        else if(is_privatecache()) cache = "private";

        if(mode ) append_str(d, "mode=" , mode , "&");
        if(cache) append_str(d, "cache=", cache, "&");
        if(vfs  ) append_str(d, "vfs="  , vfs  , "&");

        if(buf_size(d))
        {
            resize_buf(d, buf_size(d) - 1);
        }

        return d;
    }
};


// default: SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_URI | SQLITE_OPEN_EXRESCODE
//
// SQLITE_OPEN_NOMUTEX means only the bCoreMutex is enabled at most,
// so the suggested use case is one conn per thread.
//      https://www.sqlite.org/threadsafe.html
//      https://www.sqlite.org/compile.html#threadsafe
//
inline constexpr auto sqlite3_flags = sqlite3_flags_t().nomutex().privatecache().uri().exrescode();


inline constexpr auto close_sqlite3_conn = [](sqlite3* h)
{
    if(int err = sqlite3_close_v2(h))
    {
        DSK_REPORTF("sqlite3_close_v2: %s", g_sqlite3_err_cat.message(err).c_str());
    }
};


// refs:
// https://sqlite.org/lang.html
// https://sqlite.org/c3ref/intro.html
//
//
// recommended settings:
//
// suppose the default sqlite3_flags: i.e.: nomutex().privatecache().uri().exrescode()
//
//      sqlite3_conn conn;
//
// improve general io:
//
//      conn.set_journal_mode(journal_mode_wal);
//      conn.set_synchronous_mode(synchronous_normal);
//      conn.set_temp_store(temp_store_memory); // may increase memory footage
//      conn.set_cache_size(...);  // may increase memory footage
//      conn.set_chunk_size(...);  // may increase file size, suggest: [1MiB, 10MiB]
//      //conn.set_page_size(...); // at least 4KiB, which should be the default
//
// if db is only accessed by 1 thread:
//
//      conn.set_locking_mode(locking_mode_exclusive);
//
// if db is accessed by more than 1 threads:
//
//      conn.set_busy_timeout(std::chrono::seconds(2));
//
// if db is huge, and lots of connections:
//
//      conn.set_mmap_size(TiB(64));
//
// when query involving large sort:
//
//      conn.set_max_work_threads(std::thread::hardware_concurrency());
//
// for fast populate or read only database
//
//      conn.set_locking_mode(locking_mode_exclusive);
//      conn.set_journal_mode(journal_mode_off);
//      conn.set_synchronous_mode(synchronous_mode_off);
//      conn.set_cache_size(...);
//      // then, batch enough writes to match cache size in a single transaction.
// 
// see more in set_journal_mode()'s comment.
// 
// settings not recommended:
// sharedcache:
//      sharedcache adds noticeable cost on Btree, and much more contention
//      chances between concurrent transactions.
//
class sqlite3_conn : public unique_handle<sqlite3*, close_sqlite3_conn>
{
public:
    // https://www.sqlite.org/c3ref/open.html
    // https://sqlite.org/inmemorydb.html
    // https://www.sqlite.org/sharedcache.html
    //
    // if path == "", a temporary database (not TEMP db) will be opened, each conn is assured to have different db.
    // so flags.sharedcache() doesn't make sense with empty path.
    //
    // all conns opened with same non empty path and flags.sharedcache() in same process, share same page cache.
    //
    // all conns opened with same non empty path and flags.mem().sharedcache() (uri() should also on as already did by sqlite3_flags) in same process,
    // share same in-memory db, special name ":memory:" is also considered non empty path.
    //
    // see: openDatabase() sqlite3BtreeOpen() in sqlite source code.
    //
    // NOTE:
    //    'path' must be UTF-8 encoded.
    //    if path is uri, the queries in uri will overwrite corresponding flags, but these overwrites won't affect db->openFlags.
    //    check the default per connection setting in sqlite3_flags_t.
    error_code open(_byte_str_ auto const& path, sqlite3_flags_t flags = sqlite3_flags.rwc(), char const* vfs = nullptr)
    {
        if(str_empty(path) && flags.is_sharedcache())
        {
            return sqlite3_general_errc::sharedcache_on_tempdb;
        }

        sqlite3* h = nullptr;
        int err = sqlite3_open_v2(cstr_data<char>(as_cstr(path)), &h, flags.value, vfs);

        if(err)
        {
            sqlite3_close(h);
            return static_cast<sqlite3_errc>(err);
        }

        reset_handle(h);

        // return enable_extended_result_codes(true); should already be set in default sqlite3_flags
        return {};
    }

    // reset database back to empty database with no schema and no content
    // works even for a badly corrupted database file
    error_code clear()
    {
        DSK_E_TRY_ONLY(enable_config(SQLITE_DBCONFIG_RESET_DATABASE, true));
        DSK_E_TRY_ONLY(exec("VACUUM"));
        return enable_config(SQLITE_DBCONFIG_RESET_DATABASE, false);
    }


    // https://sqlite.org/lang_attach.html
    //
    // the attached db will be opened using the same flags passed to conn.open(), except those specified in uri when open(),
    // and also exclude SQLITE_OPEN_FULLMUTEX/SQLITE_OPEN_NOMUTEX, these are just connection mutex settings.
    //
    // you can overwrite conn's openFlags's ro/rw/rwc/mem/xxxcache via uri query or 'flags' which is converted to query part of uri.
    // so uri should not use ':memory:', 'mode=xxx', 'cache=xxx', if corresponding flags is set.
    //
    // see: openDatabase() sqlite3BtreeOpen() in sqlite source code.
    //
    // NOTE: the attached db must using same encoding as main;
    //       total attached dbs cannot exceed SQLITE_MAX_ATTACHED or max_attached_dbs()
    expected<> attach(_byte_str_ auto const& path, _byte_str_ auto const& schema,
                      sqlite3_flags_t flags = sqlite3_flags.clear(), char const* vfs = nullptr)
    {
        // TODO: use an actual uri parser
        auto&& pv = str_view<char>(path);

        if((pv.contains(":memory:") &&  flags.is_mem() ) ||
           (pv.contains("mode="   ) && (flags.is_ro() || flags.is_rw() || flags.is_mem ()) )
           (pv.contains("cache="  ) && (flags.is_sharedcache() || flags.is_privatecache()) )
           (pv.contains("vfs="    ) && (vfs                                              ) )
        )
        {
            return sqlite3_general_errc::incompatible_uri_flags_vfs;
        }

        string p;

        if(! pv.starts_with("file:"))
        {
            p += "file:";
        }

        p += pv;

        if(auto q = flags.to_uri_query(vfs); q.size())
        {
            p += (p.find('?') == npos ? '?' : '&');
            p += q;
        }

        return exec("ATTACH ? AS ?", p, schema);
    }

    expected<> detach(_byte_str_ auto const& schema)
    {
        return exec("DETACH ?", schema);
    }

    sqlite3_errc drop_all_modules() noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_drop_modules(checked_handle(), nullptr));
    }

    sqlite3_errc drop_all_modules_except(std::ranges::range auto const& ms)
    {
        vector<char const*> cms;
        
        cms.reserve(std::ranges::size(ms) + 1);
        
        for(auto& m : ms)
        {
            cms.emplace_back(cstr_data(m));
        }

        cms.emplace_back(nullptr);

        return static_cast<sqlite3_errc>(sqlite3_drop_modules(checked_handle(), cms.data()));
    }

    int highest_transaction_state(_byte_str_ auto const& schema)
    {
        return sqlite3_txn_state(checked_handle(), cstr_data<char>(as_cstr(schema)));
    }

    // over all schemas/dbs
    int highest_transaction_state() noexcept
    {
        return sqlite3_txn_state(checked_handle(), nullptr);
    }

    // all table/view/index/trigger infos are stored in system table sqlite_schema
    // https://www.sqlite.org/schematab.html
    // this also includes internal schema objects:
    // https://www.sqlite.org/fileformat2.html#intschema

    // https://sqlite.org/pragma.html#pragma_table_info
    expected<bool> table_exists(_byte_str_ auto const& schema, _byte_str_ auto const& tbl)
    {
        // NOTE: PRAGMA doesn't support bindings,
        //       but PRAGMAs that return results and that have no side-effects
        //       can be accessed from ordinary SELECT statements as table-valued functions(which is still experimental)
        //       PRAGMA schema.table_info(tbl) is equal to:
        //       SELECT * FROM schema.pragma_table_info('tbl')

        //return prepare(cat_str("PRAGMA ", schema, ".table_info(", tbl, ")")).next();

        // optimizer should be smart enough to stop after one record is found, so 'LIMIT 1' is not needed
        return exec<bool>(cat_str(
            "SELECT EXISTS(SELECT 1 FROM ",
                schema, ".pragma_table_info('", tbl, "'))"));
    }

    expected<bool> table_exists(_byte_str_ auto const& tbl)
    {
        return table_exists("main", tbl);
    }


    // https://sqlite.org/pragma.html#pragma_database_list
    template<class Seq = vector<string>>
    expected<Seq> db_names()
    {
        Seq dbs;

        DSK_E_TRY_FWD(ps, prepare("SELECT name from pragma_database_list()"));

        for(;;)
        {
            DSK_E_TRY(bool hasRow, ps.next(dbs.emplace_back()));

            if(! hasRow)
            {
                dbs.pop_back();
                break;
            }
        }

        return dbs;
    }

    char const* attached_db_name(int i = 0) const noexcept
    {
                                               //vvv: first 2 is 'main' and 'temp'
        return sqlite3_db_name(checked_handle(), 2 + i);
    }

    template<class T>
    expected<T> pragma_get(_byte_str_ auto const& pragma)
    {
        return exec<T>(cat_str("PRAGMA ", pragma));
    }

    template<class T>
    expected<T> pragma_get(_byte_str_ auto const& scheme, _byte_str_ auto const& pragma)
    {
        return exec<T>(cat_str("PRAGMA ", scheme, ".", pragma));
    }

    template<bool CheckResult = true, class T>
    error_code pragma_set(_byte_str_ auto const& pragma, T const& expectedResult)
    {
        constexpr bool ResultIsStr = _str_<T>;

        auto&& eRes = DSK_CONDITIONAL_V(ResultIsStr, str_view<char>(expectedResult),
                                                     expectedResult);

        char const* quote = ResultIsStr ? "'" : "";
        auto sql = cat_as_str("PRAGMA ", pragma, '=', quote, eRes, quote);

        DSK_E_TRY_ONLY(exec(sql));

        if constexpr(CheckResult)
        {
            DSK_E_TRY_FWD(actualResult, pragma_get<std::conditional_t<ResultIsStr, string, T>>(pragma));

            bool ok = [&]()
            {
                if constexpr(ResultIsStr) return ascii_iequal(actualResult, eRes);
                else                      return actualResult == eRes;
            }();

            if(! ok)
            {
                return sqlite3_general_errc::pragma_set_value_diff;
            }
        }

        return {};
    }

    template<bool CheckResult = true>
    error_code pragma_set(_byte_str_ auto const& scheme, _byte_str_ auto const& pragma, auto const& expectedResult)
    {
        return pragma_set<CheckResult>(cat_as_str(scheme, ".", pragma), expectedResult);
    }


    // https://sqlite.org/pragma.html#pragma_application_id
    error_code set_application_id(_byte_str_ auto const& schema, int32_t id) { return pragma_set(schema, "application_id", id); }
    error_code set_application_id(                               int32_t id) { return set_application_id("main", id); }

    expected<int32_t> application_id(_byte_str_ auto const& schema) { return pragma_get<int32_t>(schema, "application_id"); }
    expected<int32_t> application_id(                             ) { return application_id("main"); }

    // https://sqlite.org/pragma.html#pragma_user_version
    error_code set_user_version(_byte_str_ auto const& schema, int32_t ver) { return pragma_set(schema, "user_version", ver); }
    error_code set_user_version(                               int32_t ver) { return set_user_version("main", ver); }

    expected<int32_t> user_version(_byte_str_ auto const& schema) { return pragma_get<int32_t>(schema, "user_version"); }
    expected<int32_t> user_version(                             ) { return user_version("main"); }

    // https://sqlite.org/pragma.html#pragma_schema_version
    // incremented on schema changes, usually should not be modified by user
    error_code set_schema_version(_byte_str_ auto const& schema, int32_t ver) { return pragma_set(schema, "schema_version", ver); }
    error_code set_schema_version(                               int32_t ver) { return set_schema_version("main", ver); }

    expected<int32_t> schema_version(_byte_str_ auto const& schema) { return pragma_get<int32_t>(schema, "schema_version"); }
    expected<int32_t> schema_version(                             ) { return schema_version("main"); }

    // https://sqlite.org/pragma.html#pragma_optimize
    // https://sqlite.org/lang_analyze.html#pragopt
    // 1. Applications with short-lived database connections should run "PRAGMA optimize;" once,
    //    just prior to closing each database connection.
    // 2. Applications that use long-lived database connections should run "PRAGMA optimize=0x10002;"
    //    when the connection is first opened, and then also run "PRAGMA optimize;" periodically,
    //    perhaps once per day, or more if the database is evolving rapidly.
    // 3. All applications should run "PRAGMA optimize;" after a schema change, especially after
    //    one or more CREATE INDEX statements.
    error_code optimize(_byte_str_ auto const& schema, unsigned mask = 0xfffe) { return pragma_set<false>(schema, "optimize", mask); }
    error_code optimize(                               unsigned mask = 0xfffe) { return optimize("main", mask); }

    // all schemas/dbs
    error_code optimize_all(unsigned mask = 0xfffe) { return pragma_set<false>("optimize", mask); }


    // https://sqlite.org/pragma.html#pragma_auto_vacuum
    // NONE/0 | FULL/1 | INCREMENTAL/2
    //
    // NONE       : pages of deleted data are reused, db file never shrink.
    // FULL       : at every transaction commit, freelist pages are moved to the end of the database file.
    //              and the database file is truncated to remove the freelist pages.
    // INCREMENTAL: freelist pages are moved and truncted only when user executes PRAGMA schema.incremental_vacuum(N).
    // 
    // Auto-vacuum does not defragment the database nor repack individual database pages the way that the VACUUM command does.
    // In fact, because it moves pages around within the file, auto-vacuum can actually make fragmentation worse.
    //
    // The only advance seems to avoid the database file size from bloating if speed is not of first concern.
    //
    // NOTE: can only be set when the database is first created, as FULL and INCREMENTAL requires storing
    //       additional information that allows each database page to be traced backwards to its referrer
    error_code set_auto_vacuum(_byte_str_ auto const& schema, auto_vacuum_mode_e mode)
    {
        return pragma_set(schema, "auto_vacuum", static_cast<int>(mode));
    }

    error_code set_auto_vacuum(auto_vacuum_mode_e mode)
    {
        return set_auto_vacuum("main", mode);
    }

    expected<auto_vacuum_mode_e> auto_vacuum_mode(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(m, pragma_get<int>(schema, "auto_vacuum"));
        return static_cast<auto_vacuum_mode_e>(m);
    }

    expected<auto_vacuum_mode_e> auto_vacuum_mode()
    {
        return auto_vacuum_mode("main");
    }

    // https://sqlite.org/pragma.html#pragma_incremental_vacuum
    // only take effect when auto_vacuum mode is INCREMENTAL.
    // If 'pages' > freelist page count, or if 'pages' < 1, the entire freelist is cleared.
    error_code incremental_vacuum(_byte_str_ auto const& schema, int pages = -1)
    {
        return pragma_set<false>(schema, "incremental_vacuum", pages);
    }

    error_code incremental_vacuum(int pages = -1)
    {
        return incremental_vacuum("main", pages);
    }

    // https://sqlite.org/pragma.html#pragma_automatic_index
    error_code enable_automatic_index(bool on = true) { return pragma_set("automatic_index", on); }
    expected<bool> automatic_index_enabled() { return pragma_get<bool>("automatic_index"); }

    // https://sqlite.org/pragma.html#pragma_case_sensitive_like
    // default: disabled
    error_code enable_case_sensitive_like(bool on = true) { return pragma_set("case_sensitive_like", on); }
    expected<bool> case_sensitive_like_enabled() { return pragma_get<bool>("case_sensitive_like"); }

    // https://sqlite.org/pragma.html#pragma_cell_size_check
    // default: disabled
    error_code enable_cell_size_check(bool on = true) { return pragma_set("cell_size_check", on); }
    expected<bool> cell_size_check_enabled() { return pragma_get<bool>("cell_size_check"); }

    error_code enable_foreign_keys(bool on = true) { return pragma_set("foreign_keys", on); }
    expected<bool> foreign_keys_enabled() { return pragma_get<bool>("foreign_keys"); }

    // https://sqlite.org/pragma.html#pragma_defer_foreign_keys
    error_code defer_fk_check(bool on = true) { return pragma_set("defer_foreign_keys", on); }
    expected<bool> fk_check_deferred() { return pragma_get<bool>("defer_foreign_keys"); }

    // https://sqlite.org/pragma.html#pragma_ignore_check_constraints
    // default: always check
    error_code ignore_check_constraints(bool on = true) { return pragma_set("ignore_check_constraints", on); }
    expected<bool> check_constraints_ignored() { return pragma_get<bool>("ignore_check_constraints"); }


    // https://sqlite.org/pragma.html#pragma_cache_size
    // https://sqlite.org/c3ref/pcache_methods2.html
    // won't persist, each conn has it's own setting
    error_code set_cache_pages(_byte_str_ auto const& schema, int64_t n) { return pragma_set(schema, "cache_size", n); }
    error_code set_cache_pages(                               int64_t n) { return set_cache_pages("main", n); }

    expected<int64_t> cache_pages(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(n, pragma_get<int64_t>(schema, "cache_size"));

        if(n >= 0)
        {
            return n;
        }

        DSK_E_TRY_FWD(ps, page_size());
        return static_cast<int64_t>(std::ceil(double(-n) * 1024. / ps.count()));
    }

    expected<int64_t> cache_pages()
    {
        return cache_pages("main");
    }

    // https://sqlite.org/pragma.html#pragma_cache_size
    // https://sqlite.org/c3ref/pcache_methods2.html
    // won't persist, each conn has it's own setting
    // default: SQLITE_DEFAULT_CACHE_SIZE
    // this is the max cache size, sqlite3 default cache method allocates
    // several pages each time, until the total caches size reach this limit.
    error_code set_cache_size(_byte_str_ auto const& schema, KiB kb) { return pragma_set(schema, "cache_size", -(kb.count())); }
    error_code set_cache_size(                               KiB kb) { return set_cache_size("main", kb); }

    expected<B_> cache_size(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(n, pragma_get<int64_t>(schema, "cache_size"));

        if(n <= 0)
        {
            return KiB(-n);
        }

        DSK_E_TRY_FWD(ps, page_size());
        return ps * n;
    }

    expected<B_> cache_size()
    {
        return cache_size("main");
    }

    // https://sqlite.org/pragma.html#pragma_mmap_size
    // cannot exceed SQLITE_MAX_MMAP_SIZE or sqlite3_global.set_mmap_size(min, max)
    error_code set_mmap_size(_byte_str_ auto const& schema, B_ bytes) { return pragma_set(schema, "mmap_size", bytes.count()); }
    error_code set_mmap_size(                               B_ bytes) { return set_mmap_size("main", bytes); }

    expected<B_> mmap_size(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(s, pragma_get<int64_t>(schema, "mmap_size"));
        return B_(s);
    }

    expected<B_> mmap_size()
    {
        return mmap_size("main");
    }

    // On cache and mmap size
    //
    // Dirty pages in a single transaction would always go to cache first.
    // If they are larger than cache, some of them will be spilled to db.(check cache_spilled)
    // https://sqlite.org/atomiccommit.html#_cache_spill_prior_to_commit
    // https://sqlite.org/pragma.html#pragma_cache_spill
    //
    // While with mmap enabled, reads search both cache and mmap view, then actual db (in that case
    // the page will be put in cache).
    // So the mmap size should be large enough to avoid seeking in actual db. Considering define a
    // very large SQLITE_MAX_MMAP_SIZE and mmap_size.
    // http://sqlite.1065341.n5.nabble.com/MMAP-performance-with-databases-over-2GB-td83543.html
    // and also mind common mmap pitfalls:
    // https://www.sublimetext.com/blog/articles/use-mmap-with-care
    //
    // Conclusion:
    // The mmap may or may not improve performance depending on the mmap
    // implementation and your workload. Just try it out with a large mmap_size.
    // It's always safer to not use mmap, if portability is of concern.
    // Setting cache_size as large as possible should be a safer bet,
    // and sqlite should also have better knowledge on caching.
    // If there are lots of conns to same db, use a large enough mmap size may improve general performance.


    // https://sqlite.org/pragma.html#pragma_page_size
    // should be set on db creation or before VACUUM.
    // The page_size() will not reflect the change immediately.
    // default: 4096 bytes
    error_code set_page_size(_byte_str_ auto const& schema, B_ bytes) { return pragma_set<false>(schema, "page_size", bytes.count()); }
    error_code set_page_size(                               B_ bytes) { return set_page_size("main", bytes); }

    expected<B_> page_size(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(s, pragma_get<int64_t>(schema, "page_size"));
        return B_(s);
    }

    expected<B_> page_size()
    {
        return page_size("main");
    }

    // https://sqlite.org/pragma.html#pragma_max_page_count
    error_code set_max_page_count(_byte_str_ auto const& schema, int64_t n) { return pragma_set(schema, "max_page_count", n); }
    error_code set_max_page_count(                               int64_t n) { return set_max_page_count("main", n); }

    expected<int64_t> max_page_count(_byte_str_ auto const& schema) { return pragma_get<int64_t>(schema, "max_page_count"); }
    expected<int64_t> max_page_count(                             ) { return max_page_count("main"); }

    // https://sqlite.org/pragma.html#pragma_freelist_count
    expected<int64_t> free_pages(_byte_str_ auto const& schema) { return pragma_get<int64_t>(schema, "freelist_count"); }
    expected<int64_t> free_pages(                             ) { return free_pages("main"); }

    // https://sqlite.org/pragma.html#pragma_page_count
    expected<int64_t> total_pages(_byte_str_ auto const& schema) { return pragma_get<int64_t>(schema, "page_count"); }
    expected<int64_t> total_pages(                             ) { return total_pages("main"); }


    // https://sqlite.org/pragma.html#pragma_encoding
    // can only be used when the database is first created;
    // UTF-8 | UTF-16 | UTF-16le | UTF-16be
    // default is UTF-8
    error_code set_encoding(_byte_str_ auto const& enc)
    {
        return pragma_set("encoding", enc);
    }

    template<_resizable_byte_buf_ D = string>
    expected<D> encoding()
    {
        return pragma_get<D>("encoding");
    }

    // http://www.sqlite.org/c3ref/busy_timeout.html
    // http://www.sqlite.org/pragma.html#pragma_busy_timeout
    // sets a busy handler that sleeps for a specified amount of time when a table is locked.
    // The handler will sleep multiple times until at least "ms" milliseconds of sleeping have accumulated.
    // After at least "ms" milliseconds of sleeping, the handler returns 0 which causes sqlite3_step() to return SQLITE_BUSY.
    // Calling this routine with an argument less than or equal to zero turns off all busy handlers.
    // There can only be a single busy handler for a particular database connection at any given moment.
    // If another busy handler was defined (using sqlite3_busy_handler()) prior to calling this routine, that other busy handler is cleared.
    sqlite3_errc set_busy_timeout(std::chrono::milliseconds ms) noexcept
    {
        return static_cast<sqlite3_errc>(
            sqlite3_busy_timeout(checked_handle(), static_cast<int>(ms.count())));
    }

    // NOTE: set_busy_timeout(ms that <= 0) clears busy handle, but not the last busyTimeout which returned by this method.
    expected<std::chrono::milliseconds> busy_timeout()
    {
        DSK_E_TRY_FWD(ms, pragma_get<int>("busy_timeout"));
        return std::chrono::milliseconds(ms);
    }

    // https://sqlite.org/pragma.html#pragma_locking_mode
    // NORMAL | EXCLUSIVE
    // prefer EXCLUSIVE, when no other process will access the same schema/database.
    // default: NORMAL
    error_code set_locking_mode(_byte_str_ auto const& schema, locking_mode_e mode)
    {
        return pragma_set(schema, "locking_mode", locking_mode_to_str(mode));
    }

    error_code set_locking_mode(locking_mode_e mode)
    {
        return set_locking_mode("main", mode);
    }

    // applied to all databases(except temp), including any databases attached later
    error_code set_all_locking_mode(locking_mode_e mode)
    {
        return pragma_set("locking_mode", locking_mode_to_str(mode));
    }

    expected<locking_mode_e> locking_mode(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(m, pragma_get<string>(schema, "locking_mode"));

        if(m == "NORMAL"   ) return locking_mode_normal;
        if(m == "EXCLUSIVE") return locking_mode_exclusive;

        return sqlite3_general_errc::unknown_locking_mode;
    }

    expected<locking_mode_e> locking_mode()
    {
        return locking_mode("main");
    }

    // https://sqlite.org/pragma.html#pragma_journal_mode
    // DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF
    //
    // In rollback journal modes(DELETE | TRUNCATE | PERSIST | MEMORY), contents to be changed
    // are written to journal first, then the database is modified, on transaction commit, the
    // journal is reset, transaction is safely committed. So a write transaction blocks any other
    // transactions in rollback journal modes.
    // https://sqlite.org/lockingv3.html#rollback
    //
    // In WAL mode, changes(modified pages) are appended to wal first. On transaction commit, if wal
    // contains more pages(not used by other read transactions) than wal_autocheckpoint, these pages
    // will be move back to database file. This is called checkpointing. WAL mode supports multiple read
    // transactions and a single write transaction. WAL implements MVCC, so that each read transaction
    // starts at different time, when write transaction interleaved, can see different version of database.
    // Large wal could slow down read, as read may need look into both database and wal.
    // https://www.sqlite.org/wal.html
    //
    // If your application mostly writes lots of new rows in batch, rollback journal modes or OFF are faster.
    // If your application mostly reads, rollback journal modes or OFF are faster.
    // If either, WAL is preferred.
    //
    // rollback journal modes are recommended to be used with FULL or NORMAL synchronous mode.
    // WAL mode is recommended to be used with NORMAL synchronous mode.
    //
    // If you just want populate a database as fast as possible, set journal_mode=OFF and synchronous=OFF.
    // Batch lots of writes in a single transaction and set a large enough cache size to hold such transaction.
    //
    // OFF: disables transaction ROLLBACK.
    //
    // default: DELETE
    error_code set_journal_mode(_byte_str_ auto const& schema, journal_mode_e mode)
    {
        return pragma_set(schema, "journal_mode", journal_mode_to_str(mode));
    }

    error_code set_journal_mode(journal_mode_e mode)
    {
        return set_journal_mode("main", mode);
    }

    error_code set_all_journal_mode(journal_mode_e mode)
    {
        return pragma_set("journal_mode", journal_mode_to_str(mode));
    }

    expected<journal_mode_e> journal_mode(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(m, pragma_get<string>(schema, "journal_mode"));

        if(m == "DELETE"  ) return journal_mode_delete;
        if(m == "TRUNCATE") return journal_mode_truncate;
        if(m == "PERSIST" ) return journal_mode_persist;
        if(m == "MEMORY"  ) return journal_mode_memory;
        if(m == "WAL"     ) return journal_mode_wal;
        if(m == "OFF"     ) return journal_mode_off;

        return sqlite3_general_errc::unknown_journal_mode;
    }

    expected<journal_mode_e> journal_mode()
    {
        return journal_mode("main");
    }

    // https://sqlite.org/pragma.html#pragma_journal_size_limit
    // -1 for no limit (default)
    //  0 for minimum
    error_code set_journal_size_limit(_byte_str_ auto const& schema, B_ bytes)
    {
        return pragma_set(schema, "journal_size_limit", bytes.count());
    }

    error_code set_journal_size_limit(B_ bytes)
    {
        return set_journal_size_limit("main", bytes);
    }

    expected<B_> journal_size_limit(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(n, pragma_get<int64_t>(schema, "journal_size_limit"));
        return B_(n);
    }

    expected<B_> journal_size_limit()
    {
        return journal_size_limit("main");
    }


    // https://sqlite.org/c3ref/wal_autocheckpoint.html
    // https://sqlite.org/pragma.html#pragma_wal_autocheckpoint
    sqlite3_errc set_wal_autocheckpoint(int pages) noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_wal_autocheckpoint(checked_handle(), pages));
    }
    
    expected<int> wal_autocheckpoint()
    {
        return pragma_get<int>("wal_autocheckpoint");
    }

    // https://www.sqlite.org/c3ref/wal_checkpoint_v2.html
    // https://sqlite.org/pragma.html#pragma_wal_checkpoint
    sqlite3_expected<std::pair<int, int>> wal_checkpoint(_byte_str_ auto const& db, int mode = SQLITE_CHECKPOINT_PASSIVE)
    {
        int nLog = 0, nCkpt = 0;

        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(
            sqlite3_wal_checkpoint_v2(checked_handle(), cstr_data<char>(as_cstr(db)), mode, &nLog, &nCkpt)));

        return {expect, nLog, nCkpt};
    }

    sqlite3_errc wal_checkpoint_all(int mode = SQLITE_CHECKPOINT_PASSIVE) noexcept
    {
        return static_cast<sqlite3_errc>(
            sqlite3_wal_checkpoint_v2(checked_handle(), nullptr, mode, nullptr, nullptr));
    }


    // https://sqlite.org/pragma.html#pragma_synchronous
    //  OFF/0 | NORMAL/1 | FULL/2 | EXTRA/3
    //
    //  durabilities:
    //
    //               OFF  |     NORMAL         |  FULL  |  EXTRA
    // App crash :    ok         ok                ok       ok
    // OS/Power  :    no    WAL may rollback       ok       ok
    //                      last trxn, DELETE
    //                      may rarely corrupt.
    error_code set_synchronous_mode(_byte_str_ auto const& schema, synchronous_mode_e mode)
    {
        return pragma_set(schema, "synchronous", static_cast<int>(mode));
    }

    error_code set_synchronous_mode(synchronous_mode_e mode)
    {
        return set_synchronous_mode("main", mode);
    }

    expected<synchronous_mode_e> synchronous_mode(_byte_str_ auto const& schema)
    {
        DSK_E_TRY_FWD(m, pragma_get<int>(schema, "synchronous"));
        return static_cast<synchronous_mode_e>(m);
    }
    
    expected<synchronous_mode_e> synchronous_mode()
    {
        return synchronous_mode("main");
    }

    // https://www.sqlite.org/pragma.html#pragma_temp_store
    // DEFAULT/0 | FILE/1 | MEMORY/2
    error_code set_temp_store(temp_store_e mode)
    {
        return pragma_set("temp_store", static_cast<int>(mode));
    }

    expected<temp_store_e> temp_store()
    {
        DSK_E_TRY_FWD(m, pragma_get<int>("temp_store"));
        return static_cast<temp_store_e>(m);
    }
                                                                                                                                                              // https://www.sqlite.org/c3ref/get_autocommit.html
    // Auto-commit mode is on by default. It's disabled by a BEGIN statement and re-enabled by a COMMIT or ROLLBACK.
    bool autocommit() const noexcept
    {
        return sqlite3_get_autocommit(checked_handle()) != 0;
    }

    // return absolute path for 'db'
    char const* file_path(_byte_str_ auto const& db) const
    {
        return sqlite3_db_filename(checked_handle(), cstr_data<char>(as_cstr(db)));
    }

    char const* file_path() const
    {
        return file_path("main");
    }

    sqlite3_expected<sqlite3_pstmt> prepare(_byte_str_ auto const& sql, unsigned prepFlags = 0) noexcept
    {
        sqlite3_stmt* stmt = nullptr;

        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(
            sqlite3_prepare_v3(checked_handle(), str_data<char>(sql), str_size<int>(sql),
                               prepFlags, &stmt, nullptr)));

        return sqlite3_pstmt(stmt);
    }

    sqlite3_expected<sqlite3_pstmt> prepare_persistent(_byte_str_ auto const& sql) noexcept
    {
        return prepare(sql, SQLITE_PREPARE_PERSISTENT);
    }

    // can run zero or more UTF-8 encoded semicolon-separate SQL statements
    // if result is required, use exec() or prepare()
    sqlite3_errc exec_multi(_byte_str_ auto const& sqls)
    {
        return static_cast<sqlite3_errc>(
            sqlite3_exec(checked_handle(), cstr_data<char>(as_cstr(sqls)), nullptr, nullptr, nullptr));
    }

    // execute one SQL statement with args, optionally return specified column(s) of first row.
    // if T is not void, return the Start column of first row;
    // if T is tuple like, return columns start at Start;
    // NOTE: be careful using none ownership type like string_view,
    //       as the statement will be reset when this method return.
    template<class T = void, size_t Start = 0>
    expected<T> exec(_byte_str_ auto const& sql, auto const&... args)
    {
        DSK_E_TRY_FWD(stmt, prepare(sql));
        return stmt.template exec<T, Start>(args...);
    }

    int64_t last_insert_rowid() const noexcept { return sqlite3_last_insert_rowid(checked_handle()); }

    // return the number of rows modified, inserted or deleted
    // by the most recently completed INSERT, UPDATE or DELETE statement
    int64_t affected_rows      () const noexcept { return sqlite3_changes64(checked_handle()); }
    int64_t total_affected_rows() const noexcept { return sqlite3_total_changes64(checked_handle()); }

    int         last_err         () const noexcept { return sqlite3_errcode(checked_handle()); }
    char const* last_err_msg     () const noexcept { return sqlite3_errmsg(checked_handle()); }
    int         last_err_offset  () const noexcept { return sqlite3_error_offset(checked_handle()); }
    int         last_extended_err() const noexcept { return sqlite3_extended_errcode(checked_handle()); }

    sqlite3_errc enable_extended_result_codes(bool enable = true) noexcept
    {
        return static_cast<sqlite3_errc>(
            sqlite3_extended_result_codes(checked_handle(), enable));
    }

    void    interrupt  ()       noexcept { sqlite3_interrupt(checked_handle()); }
    bool is_interrupted() const noexcept { return sqlite3_is_interrupted(checked_handle()); }

    // use enable_load_extension_capi() to enable before use this
    sqlite3_errc load_extension(_byte_str_ auto const& file, char const* entryPoint = nullptr)
    {
        char* errMsg = nullptr;
        
        int err = sqlite3_load_extension(checked_handle(), cstr_data<char>(as_cstr(file)), entryPoint, &errMsg);
        
        if(err != SQLITE_OK)
        {
            DSK_REPORTF("sqlite3_load_extension: %s", errMsg);
            sqlite3_free(errMsg);
        }

        return static_cast<sqlite3_errc>(err);
    }

    expected<fts5_api*> fts5()
    {
        fts5_api* t = nullptr;
        
        DSK_E_TRY_ONLY(exec("SELECT fts5(?)", sqlite3_ptr_arg(&t, "fts5_api_ptr")));
        
        if(! t)
        {
            return sqlite3_general_errc::no_fts5_api;
        }

        return t;
    }

/// Low-Level Control Of VFS
// https://sqlite.org/c3ref/file_control.html
// https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html

    sqlite3_errc fcntl(_byte_str_ auto const& db, int op, void* v)
    {
        return static_cast<sqlite3_errc>(
            sqlite3_file_control(checked_handle(), cstr_data<char>(as_cstr(db)), op, v));
    }

    // https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html#sqlitefcntlchunksize
    //
    // extends and truncates the database file in chunk
    // large chunks may reduce file system level fragmentation and improve performance on some systems,
    // at the cost of larger file size.
    sqlite3_errc set_chunk_size(_byte_str_ auto const& db, B_ s)
    {
        int v = static_cast<int>(s.count());
        return fcntl(db, SQLITE_FCNTL_CHUNK_SIZE, &v);
    }

    sqlite3_errc set_chunk_size(B_ s)
    {
        return set_chunk_size("main", s);
    }

    error_code set_all_chunk_size(B_ s)
    {
        DSK_E_TRY_FWD(dbs, db_names());

        for(auto const& db : dbs)
        {
            DSK_E_TRY_ONLY(set_chunk_size(db, s));
        }

        return {};
    }

    error_code set_journal_chunk_size(_byte_str_ auto const& db, B_ s)
    {
        sqlite3_file* f = nullptr;

        DSK_E_TRY_ONLY(fcntl(db, SQLITE_FCNTL_JOURNAL_POINTER, &f));

        if(! f)
        {
            return sqlite3_general_errc::journal_not_found;
        }

        int v = static_cast<int>(s.count());

        return static_cast<sqlite3_errc>(f->pMethods->xFileControl(f, SQLITE_FCNTL_CHUNK_SIZE, &v));
    }

    error_code set_journal_chunk_size(B_ s)
    {
        return set_journal_chunk_size("main", s);
    }

    error_code set_all_journal_chunk_size(B_ s)
    {
        DSK_E_TRY_FWD(dbs, db_names());

        for(auto const& db : dbs)
        {
            DSK_E_TRY_ONLY(set_journal_chunk_size(db, s));
        }

        return {};
    }

    // https://sqlite.org/c3ref/c_fcntl_begin_atomic_write.html#sqlitefcntlvfsname
    sqlite3_expected<sqlite3_str> vfs_name(_byte_str_ auto const& db)
    {
        char* s = nullptr;
        DSK_E_TRY_ONLY(fcntl(db, SQLITE_FCNTL_VFSNAME, &s));
        return sqlite3_str(s);
    }


// config
// https://sqlite.org/c3ref/c_dbconfig_defensive.html

    sqlite3_errc config(int op, auto... a) noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_db_config(checked_handle(), op, a...));
    }

    sqlite3_errc enable_config(int op, bool enable = true) noexcept
    {
        return config(op, enable, nullptr);
    }

    sqlite3_expected<bool> config_enabled(int op) noexcept
    {
        int t;
        DSK_E_TRY_ONLY(config(op, -1, &t));
        return t;
    }

    sqlite3_errc set_lookaside(int slotSize, int nSlot, void* b = nullptr) noexcept
    {
        return config(SQLITE_DBCONFIG_LOOKASIDE, b, slotSize, nSlot);
    }

    sqlite3_errc enable_fk(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_ENABLE_FKEY, enable); }
    sqlite3_expected<bool> fk_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_ENABLE_FKEY); }

    sqlite3_errc enable_trigger(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_ENABLE_TRIGGER, enable); }
    sqlite3_expected<bool> trigger_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_ENABLE_TRIGGER); }

    sqlite3_errc enable_fts3_tokenizer(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER, enable); }
    sqlite3_expected<bool> fts3_tokenizer_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER); }

    sqlite3_errc enable_load_extension_capi(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, enable); }
    sqlite3_expected<bool> load_extension_capi_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION); }

    // enable/disable load extension from both sql and capi
    sqlite3_errc enable_load_extension_both(bool enable = true) noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_enable_load_extension(checked_handle(), enable));
    }

    sqlite3_errc set_maindb_name(_byte_str_ auto const& b) noexcept { return config(SQLITE_DBCONFIG_MAINDBNAME, cstr_data<char>(as_cstr(b))); }

    sqlite3_errc enable_ckpt_on_close(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE, !enable); }
    sqlite3_expected<bool> ckpt_on_close_enabled() noexcept { return ! config_enabled(SQLITE_DBCONFIG_NO_CKPT_ON_CLOSE); }

    sqlite3_errc enable_qpsg(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_ENABLE_QPSG, enable); }
    sqlite3_expected<bool> qpsg_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_ENABLE_QPSG); }

    sqlite3_errc enable_trigger_eqp(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_TRIGGER_EQP, enable); }
    sqlite3_expected<bool> trigger_eqp_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_TRIGGER_EQP); }

    sqlite3_errc enable_defensive(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_DEFENSIVE, enable); }
    sqlite3_expected<bool> defensive_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_DEFENSIVE); }

    sqlite3_errc enable_writable_schema(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_WRITABLE_SCHEMA, enable); }
    sqlite3_expected<bool> writable_schema_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_WRITABLE_SCHEMA); }

    sqlite3_errc enable_legacy_alter_table(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_LEGACY_ALTER_TABLE, enable); }
    sqlite3_expected<bool> legacy_alter_table_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_LEGACY_ALTER_TABLE); }

    sqlite3_errc enable_dqs_dml(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_DQS_DML, enable); }
    sqlite3_expected<bool> dqs_dml_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_DQS_DML); }

    sqlite3_errc enable_dqs_ddl(bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_DQS_DDL, enable); }
    sqlite3_expected<bool> dqs_ddl_enabled() noexcept { return config_enabled(SQLITE_DBCONFIG_DQS_DDL); }

/// limit
// https://sqlite.org/c3ref/limit.html
// https://sqlite.org/c3ref/c_limit_attached.html
    
    int limit(int id, int newVal = -1) noexcept
    {
        return sqlite3_limit(checked_handle(), id, newVal);
    }

    // currently only some ORDER BY query can take advantage of multi threaded sort.
    // default: 0, i.e. disabled.
    // http://sqlite.1065341.n5.nabble.com/how-is-quot-pragma-threads-4-quot-working-td91469.html
    // https://www.sqlite.org/c3ref/c_config_covering_index_scan.html#sqliteconfigpmasz
    int max_work_threads(int n = -1) noexcept { return limit(SQLITE_LIMIT_WORKER_THREADS, n); }

    int max_attached_dbs(int n = -1) noexcept { return limit(SQLITE_LIMIT_ATTACHED, n); }

/// status
// https://sqlite.org/c3ref/c_dbstatus_options.html

    sqlite3_expected<std::pair<int, int>> status(int op, bool reset = false) noexcept
    {
        int cur = 0, highWater = 0;

        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(
            sqlite3_db_status(checked_handle(), op, &cur, &highWater, reset)));
        
        return {expect, cur, highWater};
    }

    sqlite3_expected<int> status_cur(int op, bool reset = false) noexcept
    {
        DSK_E_TRY_FWD(s, status(op, reset));
        return s.first;
    }

    sqlite3_expected<int> status_highwater(int op, bool reset = false) noexcept
    {
        DSK_E_TRY_FWD(s, status(op, reset));
        return s.second;
    }

    sqlite3_expected<std::pair<int, int>> reset_status(int op, bool reset = true) noexcept
    {
        return status(op, reset);
    }

    sqlite3_expected<int>       lookaside_used(bool reset = false) noexcept { return status_cur(SQLITE_DBSTATUS_LOOKASIDE_USED, reset); }
    sqlite3_expected<int> reset_lookaside_used(bool reset =  true) noexcept { return lookaside_used(reset); }

    sqlite3_expected<int>       lookaside_hit(bool reset = false) noexcept { return status_highwater(SQLITE_DBSTATUS_LOOKASIDE_HIT, reset); }
    sqlite3_expected<int> reset_lookaside_hit(bool reset =  true) noexcept { return lookaside_hit(reset); }

    sqlite3_expected<int>       lookaside_miss_size(bool reset = false) noexcept { return status_highwater(SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, reset); }
    sqlite3_expected<int> reset_lookaside_miss_size(bool reset =  true) noexcept { return lookaside_miss_size(reset); }

    sqlite3_expected<int>       lookaside_miss_full(bool reset = false) noexcept { return status_highwater(SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, reset); }
    sqlite3_expected<int> reset_lookaside_miss_full(bool reset =  true) noexcept { return lookaside_miss_full(reset); }

    sqlite3_expected<int> cache_used_shared() noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_USED_SHARED); }
    sqlite3_expected<int> cache_used       () noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_USED); }

    sqlite3_expected<int> schema_used() noexcept { return status_cur(SQLITE_DBSTATUS_SCHEMA_USED); }
    sqlite3_expected<int>   stmt_used() noexcept { return status_cur(SQLITE_DBSTATUS_STMT_USED); }

    sqlite3_expected<int>       cache_spilled(bool reset = false) noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_SPILL, reset); }
    sqlite3_expected<int> reset_cache_spilled(bool reset =  true) noexcept { return cache_spilled(reset); }

    sqlite3_expected<int>       cache_hit(bool reset = false) noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_HIT, reset); }
    sqlite3_expected<int> reset_cache_hit(bool reset =  true) noexcept { return cache_hit(reset); }

    sqlite3_expected<int>       cache_miss(bool reset = false) noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_MISS, reset); }
    sqlite3_expected<int> reset_cache_miss(bool reset =  true) noexcept { return cache_miss(reset); }

    sqlite3_expected<int>       cache_written(bool reset = false) noexcept { return status_cur(SQLITE_DBSTATUS_CACHE_WRITE, reset); }
    sqlite3_expected<int> reset_cache_written(bool reset =  true) noexcept { return cache_written(reset); }

    sqlite3_expected<int> deferred_fk_checks() noexcept { return status_cur(SQLITE_DBSTATUS_DEFERRED_FKS); }

/// transaction
// https://sqlite.org/lang_transaction.html
// https://www.sqlite.org/isolation.html
// 
// Only one write transaction is allowed at any time. Other concurrent write transactions will fail with SQLITE_BUSY.
// Concurrent read transactions won't see undergoing write transaction.
// If you want to implement atomic read-modify-write operation, begin IMMEDIATE/EXCLUSIVE transactions and handle SQLITE_BUSY.
// 
// NOTE: Do not nest transactions, use savepoints.
//       Wrap read ops in a transaction can reduce overhead of obtaining and releasing shared lock.
//       You should still commit such transaction, in case you add write ops later.
//       Transaction also assures consistent reads by preserving a snapshot in the moment.

    expected<> begin(transaction_e type = transaction_deferred)
    {
        return exec(cat_str("BEGIN ", transaction_to_str(type)));
    }

    expected<> commit()
    {
        return exec("COMMIT");
    }

    expected<> rollback()
    {
        return exec("ROLLBACK");
    }

    // https://sqlite.org/lang_savepoint.html
    //
    // SAVEPOINT names are case-insensitive, and do not have to be unique.
    // Any RELEASE or ROLLBACK TO of a SAVEPOINT will act upon the most recent one with a matching name.
    //
    // SAVEPOINT merely marks the uncommitted changes, and go through them.

    expected<> savepoint(_byte_str_ auto const& name)
    {
        return exec(cat_str("SAVEPOINT ", name));
    }

    expected<> release(_byte_str_ auto const& name)
    {
        return exec(cat_str("RELEASE ", name));
    }

    expected<> rollback_to(_byte_str_ auto const& name)
    {
        return exec(cat_str("ROLLBACK TO ", name));
    }

    template<class Alloc>
    class savepoint_t
    {
        struct data_t
        {
            sqlite3_conn& conn;
            string        name;
            bool          released = false;
        };

        allocated_unique_ptr<data_t, Alloc> _d;

    public:
        savepoint_t(savepoint_t&&) = default;

        savepoint_t(sqlite3_conn& conn, _byte_str_ auto&& name)
            : _d(allocate_unique<data_t>(Alloc(), conn, as_str_of<char>(DSK_FORWARD(name))))
        {}

        ~savepoint_t()
        {
            DSK_ASSERT(! _d || _d->released);
        }

        constexpr auto operator<=>(savepoint_t const&) const = default;

        expected<> release()
        {
            DSK_ASSERT(! _d->released);
            DSK_E_TRY_ONLY(_d->conn.release(_d->name));
            _d->released = true;
            return {};
        }

        expected<> rollback_if_unreleased()
        {
            return rollback_if_unreleased_op()();
        }

        auto rollback_if_unreleased_op()
        {
            return [_d = _d.get()]() -> expected<>
            {
                if(! _d->released)
                {
                    DSK_E_TRY_ONLY(_d->conn.rollback_to(_d->name));
                    _d->released = true;
                }

                return {};
            };
        }
    };

    // as this returns _async_op_, use DSK_TRY/DSK_WAIT
    template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
    auto make_savepoint(_byte_str_ auto&& name)
    {
        return sync_async_op([this, name = DSK_FORWARD(name)](_async_ctx_ auto&& ctx) mutable -> expected<savepoint_t<Alloc>>
        {
            savepoint_t<Alloc> sp(*this, mut_move(name));

            DSK_E_TRY_ONLY(savepoint(name));
            add_cleanup(ctx, sync_async_op(sp.rollback_if_unreleased_op()));

            return sp;
        });
    }

    // as this returns _async_op_, use DSK_TRY/DSK_WAIT
    template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
    auto make_savepoint()
    {
        return make_savepoint<Alloc>("sp");
    }

    template<class Alloc>
    class transaction_t
    {
        struct data_t
        {
            sqlite3_conn& conn;
            transaction_e type;
            bool done = false;
        };

        allocated_unique_ptr<data_t, Alloc> _d;

    public:
        transaction_t(transaction_t&&) = default;

        explicit transaction_t(sqlite3_conn& conn, transaction_e type)
            : _d(allocate_unique<data_t>(Alloc(), conn, type))
        {}

        ~transaction_t()
        {
            DSK_ASSERT(! _d || _d->done);
        }

        constexpr auto operator<=>(transaction_t const&) const = default;

        expected<> commit()
        {
            DSK_ASSERT(! _d->done);
            DSK_E_TRY_ONLY(_d->conn.commit());
            _d->done = true;
            return {};
        }

        expected<> commit_and_begin()
        {
            DSK_ASSERT(! _d->done);
            DSK_E_TRY_ONLY(_d->conn.commit());
            DSK_E_TRY_ONLY(_d->conn.begin(_d->type));
            return {};
        }

        expected<> rollback()
        {
            DSK_ASSERT(! _d->done);
            DSK_E_TRY_ONLY(_d->conn.rollback());
            _d->done = true;
            return {};
        }

        expected<> rollback_if_uncommited()
        {
            return rollback_if_uncommited_op()();
        }

        auto rollback_if_uncommited_op()
        {
            return [_d = _d.get()]() -> expected<>
            {
                if(! _d->done)
                {
                    DSK_E_TRY_ONLY(_d->conn.rollback());
                    _d->done = true;
                }

                return {};
            };
        }

        expected<> savepoint(_byte_str_ auto const& name)
        {
            DSK_ASSERT(! _d->done);
            return _d->conn.savepoint(name);
        }

        void release(_byte_str_ auto const& name)
        {
            DSK_ASSERT(! _d->done);
            return _d->conn.release(name);
        }

        void rollback_to(_byte_str_ auto const& name)
        {
            DSK_ASSERT(! _d->done);
            return _d->conn.rollback_to(name);
        }

        // as this returns _async_op_, use DSK_TRY/DSK_WAIT
        auto make_savepoint(_byte_str_ auto&& name)
        {
            DSK_ASSERT(! _d->done);
            return _d->conn.template make_savepoint<Alloc>(DSK_FORWARD(name));
        }

        // as this returns _async_op_, use DSK_TRY/DSK_WAIT
        auto make_savepoint()
        {
            DSK_ASSERT(! _d->done);
            return _d->conn.template make_savepoint<Alloc>();
        }
    };

    // as this returns _async_op_, use DSK_TRY/DSK_WAIT
    template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
    auto make_transaction(transaction_e type = transaction_deferred)
    {
        return sync_async_op([this, type](_async_ctx_ auto&& ctx) -> expected<transaction_t<Alloc>>
        {
            DSK_E_TRY_ONLY(begin(type));

            transaction_t<Alloc> trxn(*this, type);

            add_cleanup(ctx, sync_async_op(trxn.rollback_if_uncommited_op()));

            return trxn;
        });
    }
};


inline constexpr auto finish_sqlite3_db_backup = [](sqlite3_backup* h)
{
    if(int err = sqlite3_backup_finish(h))
    {
        DSK_REPORTF("sqlite3_backup_finish: %s", g_sqlite3_err_cat.message(err).c_str());
    }
};


// https://sqlite.org/backup.html
// https://sqlite.org/c3ref/backup_finish.html
class sqlite3_db_backup : public
    unique_handle<sqlite3_backup*, finish_sqlite3_db_backup>
{
public:
    sqlite3_errc init(sqlite3_conn& src, _byte_str_ auto const& srcDb,
                      sqlite3_conn& dst, _byte_str_ auto const& dstDb)
    {
        sqlite3_backup* h = sqlite3_backup_init(dst.checked_handle(), cstr_data<char>(as_cstr(dstDb)),
                                                src.checked_handle(), cstr_data<char>(as_cstr(srcDb)));

        if(! h)
        {
            return static_cast<sqlite3_errc>(dst.last_extended_err());
        }

        reset_handle(h);
        return {};
    }

    sqlite3_errc init(sqlite3_conn& src, sqlite3_conn& dst)
    {
        return init(src, "main", dst, "main");
    }

    // return if there is more pages to process
    sqlite3_expected<bool> step(int nPage) noexcept
    {
        int r = sqlite3_backup_step(checked_handle(), nPage);

        if(r == SQLITE_OK  ) return true;
        if(r == SQLITE_DONE) return false;

        return static_cast<sqlite3_errc>(r);
    }

    sqlite3_expected<bool> step_all() noexcept
    {
        return step(-1);
    }

    // source db remaining and total page count, only update on step()
    int remaining() const noexcept { return sqlite3_backup_remaining(checked_handle()); }
    int pagecount() const noexcept { return sqlite3_backup_pagecount(checked_handle()); }
};


} // namespace std
