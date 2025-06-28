#pragma once

#include <dsk/err.hpp>
#include <dsk/expected.hpp>
#include <dsk/util/str.hpp>
#include <sqlite3.h>


namespace dsk{


enum class [[nodiscard]] sqlite3_general_errc
{
    no_more_row = 1,
    sharedcache_on_tempdb,
    journal_not_found,
    incompatible_uri_flags_vfs,
    pragma_set_value_diff,
    unknown_locking_mode,
    unknown_journal_mode,
    no_fts5_api,
};


class sqlite3_general_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "sqlite3_general"; }

    std::string message(int condition) const override
    {
        switch(static_cast<sqlite3_general_errc>(condition))
        {
            case sqlite3_general_errc::no_more_row                : return "no more row";
            case sqlite3_general_errc::sharedcache_on_tempdb      : return "sharedcache cannot be applied to empty path (i.e. temporary db)";
            case sqlite3_general_errc::journal_not_found          : return "journal not found";
            case sqlite3_general_errc::incompatible_uri_flags_vfs : return "uri conflict with flags or vfs";
            case sqlite3_general_errc::pragma_set_value_diff      : return "pragma_set results in different value than expected";
            case sqlite3_general_errc::unknown_locking_mode       : return "unknown locking mode";
            case sqlite3_general_errc::unknown_journal_mode       : return "unknown journal mode";
            case sqlite3_general_errc::no_fts5_api                : return "no fts5 api";
        }

        return "undefined";
    }
};


inline constexpr sqlite3_general_err_category g_sqlite3_general_err_cat;


static_assert(SQLITE_OK == 0);

enum class [[nodiscard]] sqlite3_errc
{
    error      = SQLITE_ERROR,
    internal   = SQLITE_INTERNAL,
    perm       = SQLITE_PERM,
    abort      = SQLITE_ABORT,
    busy       = SQLITE_BUSY,
    locked     = SQLITE_LOCKED,
    nomem      = SQLITE_NOMEM,
    readonly   = SQLITE_READONLY,
    interrupt  = SQLITE_INTERRUPT,
    ioerr      = SQLITE_IOERR,
    corrupt    = SQLITE_CORRUPT,
    notfound   = SQLITE_NOTFOUND,
    full       = SQLITE_FULL,
    cantopen   = SQLITE_CANTOPEN,
    protocol   = SQLITE_PROTOCOL,
    empty      = SQLITE_EMPTY,
    schema     = SQLITE_SCHEMA,
    toobig     = SQLITE_TOOBIG,
    constraint = SQLITE_CONSTRAINT,
    mismatch   = SQLITE_MISMATCH,
    misuse     = SQLITE_MISUSE,
    nolfs      = SQLITE_NOLFS,
    auth       = SQLITE_AUTH,
    format     = SQLITE_FORMAT,
    range      = SQLITE_RANGE,
    notadb     = SQLITE_NOTADB,
    notice     = SQLITE_NOTICE,
    warning    = SQLITE_WARNING,
    row        = SQLITE_ROW,
    done       = SQLITE_DONE,

    /// extended

    error_missing_collseq    = SQLITE_ERROR_MISSING_COLLSEQ,
    error_retry              = SQLITE_ERROR_RETRY,
    error_snapshot           = SQLITE_ERROR_SNAPSHOT,
    ioerr_read               = SQLITE_IOERR_READ,
    ioerr_short_read         = SQLITE_IOERR_SHORT_READ,
    ioerr_write              = SQLITE_IOERR_WRITE,
    ioerr_fsync              = SQLITE_IOERR_FSYNC,
    ioerr_dir_fsync          = SQLITE_IOERR_DIR_FSYNC,
    ioerr_truncate           = SQLITE_IOERR_TRUNCATE,
    ioerr_fstat              = SQLITE_IOERR_FSTAT,
    ioerr_unlock             = SQLITE_IOERR_UNLOCK,
    ioerr_rdlock             = SQLITE_IOERR_RDLOCK,
    ioerr_delete             = SQLITE_IOERR_DELETE,
    ioerr_blocked            = SQLITE_IOERR_BLOCKED,
    ioerr_nomem              = SQLITE_IOERR_NOMEM,
    ioerr_access             = SQLITE_IOERR_ACCESS,
    ioerr_checkreservedlock  = SQLITE_IOERR_CHECKRESERVEDLOCK,
    ioerr_lock               = SQLITE_IOERR_LOCK,
    ioerr_close              = SQLITE_IOERR_CLOSE,
    ioerr_dir_close          = SQLITE_IOERR_DIR_CLOSE,
    ioerr_shmopen            = SQLITE_IOERR_SHMOPEN,
    ioerr_shmsize            = SQLITE_IOERR_SHMSIZE,
    ioerr_shmlock            = SQLITE_IOERR_SHMLOCK,
    ioerr_shmmap             = SQLITE_IOERR_SHMMAP,
    ioerr_seek               = SQLITE_IOERR_SEEK,
    ioerr_delete_noent       = SQLITE_IOERR_DELETE_NOENT,
    ioerr_mmap               = SQLITE_IOERR_MMAP,
    ioerr_gettemppath        = SQLITE_IOERR_GETTEMPPATH,
    ioerr_convpath           = SQLITE_IOERR_CONVPATH,
    ioerr_vnode              = SQLITE_IOERR_VNODE,
    ioerr_auth               = SQLITE_IOERR_AUTH,
    ioerr_begin_atomic       = SQLITE_IOERR_BEGIN_ATOMIC,
    ioerr_commit_atomic      = SQLITE_IOERR_COMMIT_ATOMIC,
    ioerr_rollback_atomic    = SQLITE_IOERR_ROLLBACK_ATOMIC,
    ioerr_data               = SQLITE_IOERR_DATA,
    ioerr_corruptfs          = SQLITE_IOERR_CORRUPTFS,
    ioerr_in_page            = SQLITE_IOERR_IN_PAGE,
    locked_sharedcache       = SQLITE_LOCKED_SHAREDCACHE,
    locked_vtab              = SQLITE_LOCKED_VTAB,
    busy_recovery            = SQLITE_BUSY_RECOVERY,
    busy_snapshot            = SQLITE_BUSY_SNAPSHOT,
    busy_timeout             = SQLITE_BUSY_TIMEOUT,
    cantopen_notempdir       = SQLITE_CANTOPEN_NOTEMPDIR,
    cantopen_isdir           = SQLITE_CANTOPEN_ISDIR,
    cantopen_fullpath        = SQLITE_CANTOPEN_FULLPATH,
    cantopen_convpath        = SQLITE_CANTOPEN_CONVPATH,
    cantopen_dirtywal        = SQLITE_CANTOPEN_DIRTYWAL,
    cantopen_symlink         = SQLITE_CANTOPEN_SYMLINK,
    corrupt_vtab             = SQLITE_CORRUPT_VTAB,
    corrupt_sequence         = SQLITE_CORRUPT_SEQUENCE,
    corrupt_index            = SQLITE_CORRUPT_INDEX,
    readonly_recovery        = SQLITE_READONLY_RECOVERY,
    readonly_cantlock        = SQLITE_READONLY_CANTLOCK,
    readonly_rollback        = SQLITE_READONLY_ROLLBACK,
    readonly_dbmoved         = SQLITE_READONLY_DBMOVED,
    readonly_cantinit        = SQLITE_READONLY_CANTINIT,
    readonly_directory       = SQLITE_READONLY_DIRECTORY,
    abort_rollback           = SQLITE_ABORT_ROLLBACK,
    constraint_check         = SQLITE_CONSTRAINT_CHECK,
    constraint_commithook    = SQLITE_CONSTRAINT_COMMITHOOK,
    constraint_foreignkey    = SQLITE_CONSTRAINT_FOREIGNKEY,
    constraint_function      = SQLITE_CONSTRAINT_FUNCTION,
    constraint_notnull       = SQLITE_CONSTRAINT_NOTNULL,
    constraint_primarykey    = SQLITE_CONSTRAINT_PRIMARYKEY,
    constraint_trigger       = SQLITE_CONSTRAINT_TRIGGER,
    constraint_unique        = SQLITE_CONSTRAINT_UNIQUE,
    constraint_vtab          = SQLITE_CONSTRAINT_VTAB,
    constraint_rowid         = SQLITE_CONSTRAINT_ROWID,
    constraint_pinned        = SQLITE_CONSTRAINT_PINNED,
    constraint_datatype      = SQLITE_CONSTRAINT_DATATYPE,
    notice_recover_wal       = SQLITE_NOTICE_RECOVER_WAL,
    notice_recover_rollback  = SQLITE_NOTICE_RECOVER_ROLLBACK,
    notice_rbu               = SQLITE_NOTICE_RBU,
    warning_autoindex        = SQLITE_WARNING_AUTOINDEX,
    auth_user                = SQLITE_AUTH_USER,
    ok_load_permanently      = SQLITE_OK_LOAD_PERMANENTLY,
    ok_symlink               = SQLITE_OK_SYMLINK,
};


template<class T>
using sqlite3_expected = expected<T, sqlite3_errc>;


class sqlite3_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "sqlite3"; }

    std::string message(int condition) const override
    {
        switch(static_cast<sqlite3_errc>(condition))
        {
            case sqlite3_errc::error     : return "SQL logic error";
            case sqlite3_errc::internal  : return "SQLITE_INTERNAL";
            case sqlite3_errc::perm      : return "access permission denied";
            case sqlite3_errc::abort     : return "query aborted";
            case sqlite3_errc::busy      : return "database is locked";
            case sqlite3_errc::locked    : return "database table is locked";
            case sqlite3_errc::nomem     : return "out of memory";
            case sqlite3_errc::readonly  : return "attempt to write a readonly database";
            case sqlite3_errc::interrupt : return "interrupted";
            case sqlite3_errc::ioerr     : return "disk I/O error";
            case sqlite3_errc::corrupt   : return "database disk image is malformed";
            case sqlite3_errc::notfound  : return "unknown operation";
            case sqlite3_errc::full      : return "database or disk is full";
            case sqlite3_errc::cantopen  : return "unable to open database file";
            case sqlite3_errc::protocol  : return "locking protocol";
            case sqlite3_errc::empty     : return "SQLITE_EMPTY";
            case sqlite3_errc::schema    : return "database schema has changed";
            case sqlite3_errc::toobig    : return "string or blob too big";
            case sqlite3_errc::constraint: return "constraint failed";
            case sqlite3_errc::mismatch  : return "datatype mismatch";
            case sqlite3_errc::misuse    : return "bad parameter or other API misuse";
            case sqlite3_errc::nolfs     : return "large file support is disabled";
            case sqlite3_errc::auth      : return "authorization denied";
            case sqlite3_errc::format    : return "SQLITE_FORMAT";
            case sqlite3_errc::range     : return "column index out of range";
            case sqlite3_errc::notadb    : return "file is not a database";
            case sqlite3_errc::notice    : return "notification message";
            case sqlite3_errc::warning   : return "warning message";
            case sqlite3_errc::row       : return "another row available";
            case sqlite3_errc::done      : return "no more rows available";

            case sqlite3_errc::error_missing_collseq  : return "error: missing collating sequence";
            case sqlite3_errc::error_retry            : return "error: retry";
            case sqlite3_errc::error_snapshot         : return "error: snapshot";
            case sqlite3_errc::ioerr_read             : return "ioerr: read";
            case sqlite3_errc::ioerr_short_read       : return "ioerr: short read";
            case sqlite3_errc::ioerr_write            : return "ioerr: write";
            case sqlite3_errc::ioerr_fsync            : return "ioerr: fsync";
            case sqlite3_errc::ioerr_dir_fsync        : return "ioerr: dir fsync";
            case sqlite3_errc::ioerr_truncate         : return "ioerr: truncate";
            case sqlite3_errc::ioerr_fstat            : return "ioerr: fstat";
            case sqlite3_errc::ioerr_unlock           : return "ioerr: unlock";
            case sqlite3_errc::ioerr_rdlock           : return "ioerr: rdlock";
            case sqlite3_errc::ioerr_delete           : return "ioerr: delete";
            case sqlite3_errc::ioerr_blocked          : return "ioerr: blocked";
            case sqlite3_errc::ioerr_nomem            : return "ioerr: nomem";
            case sqlite3_errc::ioerr_access           : return "ioerr: access";
            case sqlite3_errc::ioerr_checkreservedlock: return "ioerr: check reserved lock";
            case sqlite3_errc::ioerr_lock             : return "ioerr: lock";
            case sqlite3_errc::ioerr_close            : return "ioerr: close";
            case sqlite3_errc::ioerr_dir_close        : return "ioerr: dir close";
            case sqlite3_errc::ioerr_shmopen          : return "ioerr: shmopen";
            case sqlite3_errc::ioerr_shmsize          : return "ioerr: shmsize";
            case sqlite3_errc::ioerr_shmlock          : return "ioerr: shmlock";
            case sqlite3_errc::ioerr_shmmap           : return "ioerr: shmmap";
            case sqlite3_errc::ioerr_seek             : return "ioerr: seek";
            case sqlite3_errc::ioerr_delete_noent     : return "ioerr: delete no ent";
            case sqlite3_errc::ioerr_mmap             : return "ioerr: mmap";
            case sqlite3_errc::ioerr_gettemppath      : return "ioerr: get temp path";
            case sqlite3_errc::ioerr_convpath         : return "ioerr: conv path";
            case sqlite3_errc::ioerr_vnode            : return "ioerr: vnode";
            case sqlite3_errc::ioerr_auth             : return "ioerr: auth";
            case sqlite3_errc::ioerr_begin_atomic     : return "ioerr: begin atomic";
            case sqlite3_errc::ioerr_commit_atomic    : return "ioerr: commit atomic";
            case sqlite3_errc::ioerr_rollback_atomic  : return "ioerr: rollback atomic";
            case sqlite3_errc::ioerr_data             : return "ioerr: data";
            case sqlite3_errc::ioerr_corruptfs        : return "ioerr: corrupt fs";
            case sqlite3_errc::ioerr_in_page          : return "ioerr: in page";
            case sqlite3_errc::locked_sharedcache     : return "locked sharedcache";
            case sqlite3_errc::locked_vtab            : return "locked vtab";
            case sqlite3_errc::busy_recovery          : return "busy recovery";
            case sqlite3_errc::busy_snapshot          : return "busy snapshot";
            case sqlite3_errc::busy_timeout           : return "busy timeout";
            case sqlite3_errc::cantopen_notempdir     : return "cant open notempdir";
            case sqlite3_errc::cantopen_isdir         : return "cant open isdir";
            case sqlite3_errc::cantopen_fullpath      : return "cant open fullpath";
            case sqlite3_errc::cantopen_convpath      : return "cant open convpath";
            case sqlite3_errc::cantopen_dirtywal      : return "cant open dirtywal";
            case sqlite3_errc::cantopen_symlink       : return "cant open symlink";
            case sqlite3_errc::corrupt_vtab           : return "corrupt vtab";
            case sqlite3_errc::corrupt_sequence       : return "corrupt sequence";
            case sqlite3_errc::corrupt_index          : return "corrupt index";
            case sqlite3_errc::readonly_recovery      : return "readonly: recovery";
            case sqlite3_errc::readonly_cantlock      : return "readonly: cant lock";
            case sqlite3_errc::readonly_rollback      : return "readonly: rollback";
            case sqlite3_errc::readonly_dbmoved       : return "readonly: db moved";
            case sqlite3_errc::readonly_cantinit      : return "readonly: cant init";
            case sqlite3_errc::readonly_directory     : return "readonly: directory";
            case sqlite3_errc::abort_rollback         : return "abort due to ROLLBACK";
            case sqlite3_errc::constraint_check       : return "constraint: check";
            case sqlite3_errc::constraint_commithook  : return "constraint: commit hook";
            case sqlite3_errc::constraint_foreignkey  : return "constraint: foreign key";
            case sqlite3_errc::constraint_function    : return "constraint: function";
            case sqlite3_errc::constraint_notnull     : return "constraint: not null";
            case sqlite3_errc::constraint_primarykey  : return "constraint: primary key";
            case sqlite3_errc::constraint_trigger     : return "constraint: trigger";
            case sqlite3_errc::constraint_unique      : return "constraint: unique";
            case sqlite3_errc::constraint_vtab        : return "constraint: vtab";
            case sqlite3_errc::constraint_rowid       : return "constraint: rowid";
            case sqlite3_errc::constraint_pinned      : return "constraint: pinned";
            case sqlite3_errc::constraint_datatype    : return "constraint: datatype";
            case sqlite3_errc::notice_recover_wal     : return "notice: recover wal";
            case sqlite3_errc::notice_recover_rollback: return "notice: recover rollback";
            case sqlite3_errc::notice_rbu             : return "notice: rbu";
            case sqlite3_errc::warning_autoindex      : return "warning: automatic index";
            case sqlite3_errc::auth_user              : return "auth: user";
            case sqlite3_errc::ok_load_permanently    : return "ok: load permanently";
            case sqlite3_errc::ok_symlink             : return "ok: symlink";
        }

        return "undefined";
    }
};


inline constexpr sqlite3_err_category g_sqlite3_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, sqlite3_general_errc, g_sqlite3_general_err_cat)
DSK_REGISTER_ERROR_CODE_ENUM(dsk, sqlite3_errc, g_sqlite3_err_cat)
