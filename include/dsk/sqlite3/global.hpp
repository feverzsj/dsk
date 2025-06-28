#pragma once

#include <dsk/sqlite3/err.hpp>
#include <dsk/util/unit.hpp>
#include <sqlite3.h>


namespace dsk{


class sqlite3_global_t
{
public:
// status
// https://sqlite.org/c3ref/c_status_malloc_count.html

    static sqlite3_expected<std::pair<int64_t, int64_t>> status(int op, bool reset = false) noexcept
    {
        sqlite3_int64 cur = 0, highWater = 0;
        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(sqlite3_status64(op, &cur, &highWater, reset)));
        return {expect, cur, highWater};
    }

    static sqlite3_expected<int64_t> status_cur(int op, bool reset = false) noexcept
    {
        DSK_E_TRY_FWD(s, status(op, reset));
        return s.first;
    }

    static sqlite3_expected<int64_t> status_highwater(int op, bool reset = false) noexcept
    {
        DSK_E_TRY_FWD(s, status(op, reset));
        return s.second;
    }

    static sqlite3_expected<std::pair<int64_t, int64_t>> reset_status(int op, bool reset = true) noexcept
    {
        return status(op, reset);
    }

    static sqlite3_expected<int64_t> malloc_size(bool reset = false) { return status_highwater(SQLITE_STATUS_MALLOC_SIZE, reset); }
    static sqlite3_expected<int64_t> reset_malloc_size(bool reset = true) { return malloc_size(reset); }

    static sqlite3_expected<int64_t> malloc_count() { return status_cur(SQLITE_STATUS_MALLOC_COUNT); }
    static sqlite3_expected<int64_t> pagecache_used() { return status_cur(SQLITE_STATUS_PAGECACHE_USED); }
    static sqlite3_expected<int64_t> pagecache_overflow() { return status_cur(SQLITE_STATUS_PAGECACHE_OVERFLOW); }

    static sqlite3_expected<int64_t> pagecache_size(bool reset = false) { return status_highwater(SQLITE_STATUS_PAGECACHE_SIZE, reset); }
    static sqlite3_expected<int64_t> reset_pagecache_size(bool reset = true) { return pagecache_size(reset); }

#ifdef YYTRACKMAXSTACKDEPTH
    static sqlite3_expected<int64_t> parser_stack(bool reset = false) { return status_highwater(SQLITE_STATUS_PARSER_STACK, reset); }
    static sqlite3_expected<int64_t> reset_parser_stack(bool reset = true) { return parser_stack(reset); }
#endif

    static int64_t memory_used() noexcept { return sqlite3_memory_used(); }
    static int64_t memory_highwater(bool reset = false) noexcept { return sqlite3_memory_highwater(reset); }
    static int64_t reset_memory_highwater(bool reset = true) noexcept { return memory_highwater(reset); }

// config
// https://sqlite.org/c3ref/c_config_covering_index_scan.html

    template<class... A>
    static sqlite3_errc config(int op, A... a) noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_config(op, a...));
    }

    static sqlite3_errc enable_config(int op, bool enable = true) noexcept
    {
        return config(op, static_cast<int>(enable));
    }

    // https://www.sqlite.org/threadsafe.html
    // https://www.sqlite.org/compile.html#threadsafe
    static sqlite3_errc use_singlethread() noexcept { return config(SQLITE_CONFIG_SINGLETHREAD); } // bCoreMutex = 0; bFullMutex = 0;
    static sqlite3_errc use_serialized  () noexcept { return config(SQLITE_CONFIG_SERIALIZED  ); } // bCoreMutex = 1; bFullMutex = 1; same as SQLITE_OPEN_FULLMUTEX
    static sqlite3_errc use_multithread () noexcept { return config(SQLITE_CONFIG_MULTITHREAD ); } // bCoreMutex = 1; bFullMutex = 0; same as SQLITE_OPEN_NOMUTEX

    static sqlite3_errc enable_uri                (bool enable = true) noexcept { return enable_config(SQLITE_CONFIG_URI, enable); }
    static sqlite3_errc enable_defensive          (bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_DEFENSIVE, enable); }
    static sqlite3_errc prefer_small_malloc       (bool prefer = true) noexcept { return enable_config(SQLITE_CONFIG_SMALL_MALLOC, prefer); }
    static sqlite3_errc enable_malloc_statics     (bool enable = true) noexcept { return enable_config(SQLITE_CONFIG_MEMSTATUS, enable); }
    static sqlite3_errc enable_trusted_schema     (bool enable = true) noexcept { return enable_config(SQLITE_DBCONFIG_TRUSTED_SCHEMA, enable); }
    static sqlite3_errc enable_covering_index_scan(bool enable = true) noexcept { return enable_config(SQLITE_CONFIG_COVERING_INDEX_SCAN, enable); }

    static sqlite3_errc set_pagecache   (void* b, int sz, int n) noexcept { return config(SQLITE_CONFIG_PAGECACHE, b, sz, n); }
    static sqlite3_errc set_lookaside   (int sz, int n         ) noexcept { return config(SQLITE_CONFIG_LOOKASIDE, sz, n); }
    static sqlite3_errc set_min_pma_size(unsigned n            ) noexcept { return config(SQLITE_CONFIG_PMASZ, n); }

    static sqlite3_errc set_heap(void* b, B_ s, B_ min ) noexcept
    {
        return config(SQLITE_CONFIG_HEAP, b, static_cast<int>(s.count()), static_cast<int>(min.count()));
    }

    // https://www.sqlite.org/c3ref/c_config_covering_index_scan.html#sqliteconfigmmapsize
    // https://www.sqlite.org/pragma.html#pragma_mmap_size
    static sqlite3_errc set_mmap_size(B_ min, B_ max) noexcept { return config(SQLITE_CONFIG_MMAP_SIZE, min.count(), max.count()); }

    static sqlite3_errc set_stmtjrnl_spill(B_ bytes) noexcept { return config(SQLITE_CONFIG_STMTJRNL_SPILL, static_cast<int>(bytes.count())); }
    static sqlite3_errc set_sorterref_size(B_ bytes) noexcept { return config(SQLITE_CONFIG_SORTERREF_SIZE, static_cast<int>(bytes.count())); }
    static sqlite3_errc set_memdb_maxsize (B_ bytes) noexcept { return config(SQLITE_CONFIG_MEMDB_MAXSIZE, bytes.count()); }

    static sqlite3_errc set_mem_methods(sqlite3_mem_methods const& m) noexcept { return config(SQLITE_CONFIG_MALLOC, &m); }
    static sqlite3_errc get_mem_methods(sqlite3_mem_methods&       m) noexcept { return config(SQLITE_CONFIG_GETMALLOC, &m); }

    static sqlite3_errc set_mutex_methods(sqlite3_mutex_methods const& m) noexcept { return config(SQLITE_CONFIG_MUTEX, &m); }
    static sqlite3_errc get_mutex_methods(sqlite3_mutex_methods&       m) noexcept { return config(SQLITE_CONFIG_GETMUTEX, &m); }

    static sqlite3_errc set_pcache_methods(sqlite3_pcache_methods2 const& m) noexcept { return config(SQLITE_CONFIG_PCACHE2, &m); }
    static sqlite3_errc get_pcache_methods(sqlite3_pcache_methods2&       m) noexcept { return config(SQLITE_CONFIG_GETPCACHE2, &m); }

    static sqlite3_expected<int> pcahce_hdrsz() noexcept
    {
        int sz = 0;
        DSK_E_TRY_ONLY(config(SQLITE_CONFIG_PCACHE_HDRSZ, &sz));
        return sz;
    }

    static sqlite3_errc reset_errlog_callback(void(*cb)(void*, int, char const*) = nullptr, void* d = nullptr) noexcept
    {
        return config(SQLITE_CONFIG_LOG, cb, d);
    }

#ifdef SQLITE_ENABLE_SQLLOG
    static sqlite3_errc reset_sqllog_callback(void(*cb)(void*, sqlite3*, char const*, int) = nullptr, void* d = nullptr) noexcept
    {
        return config(SQLITE_CONFIG_SQLLOG, cb, d);
    }
#endif

#ifdef SQLITE_WIN32_MALLOC
    static sqlite3_errc set_win32_heapsize(B_ bytes) noexcept
    {
        return config(SQLITE_CONFIG_WIN32_HEAPSIZE, static_cast<int>(bytes.count()));
    }
#endif

    // returns previous value
    static int64_t set_soft_heap_limit(B_ bytes) noexcept { return sqlite3_soft_heap_limit64(bytes.count()); }
    static int64_t set_hard_heap_limit(B_ bytes) noexcept { return sqlite3_hard_heap_limit64(bytes.count()); }

    static sqlite3_errc enable_shared_cache(bool enable = true) noexcept
    {
        return static_cast<sqlite3_errc>(sqlite3_enable_shared_cache(enable));
    }

#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    template<class Seq = vector<char const*>>
    static Seq used_compile_options()
    {
        Seq opts;
        int i = 0;

        while(char const* o = sqlite3_compileoption_get(i++))
        {
            opts.emplace_back(o);
        }

        return opts;
    }

    static bool compile_option_used(char const* o) noexcept
    {
        return sqlite3_compileoption_used(o);
    }
#endif
};


inline constexpr sqlite3_global_t sqlite3_global;


} // namespace dsk
