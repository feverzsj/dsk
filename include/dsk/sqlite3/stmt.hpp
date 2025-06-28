#pragma once

#include <dsk/util/str.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/util/num_cast.hpp>
#include <sqlite3.h>


namespace dsk{


class sqlite3_pstmt;


inline constexpr struct bind_sqlite3_arg_cpo
{
    constexpr auto operator()(sqlite3_pstmt& p, int i, auto&& a) const
    requires(requires{
        dsk_bind_sqlite3_arg(*this, p, i, DSK_FORWARD(a)); })
    {
        return dsk_bind_sqlite3_arg(*this, p, i, DSK_FORWARD(a));
    }
} bind_sqlite3_arg;


inline constexpr struct get_sqlite3_col_cpo
{
    constexpr auto operator()(sqlite3_pstmt& p, int i, auto&& v) const
    requires(requires{
        dsk_get_sqlite3_col(*this, p, i, DSK_FORWARD(v)); })
    {
        return dsk_get_sqlite3_col(*this, p, i, DSK_FORWARD(v));
    }
} get_sqlite3_col;


class sqlite3_str : public unique_handle<char*, sqlite3_free>
{
    using base = unique_handle<char*, sqlite3_free>;

public:
    using base::base;

    char* c_str() const noexcept { return handle(); }
    char*  data() const noexcept { return handle(); }
    size_t size() const noexcept { return strlen(handle()); }
};


struct sqlite3_ptr_arg
{
    void* p;
    char const* t = nullptr;
};


inline constexpr auto finalize_sqlite3_pstmt = [](sqlite3_stmt* h) noexcept
{
    // If sqlite3_finalize()/sqlite3_reset() return an error,
    // it's either the same as last sqlite3_step() which was failed,
    // or SQLITE_BUSY if locking constraints prevent the database
    // change from committing.
    // The operation itself should always succeed.
    if(int err = sqlite3_finalize(h))
    {
        DSK_REPORTF("sqlite3_finalize: %s", g_sqlite3_err_cat.message(err).c_str());
    }
};


// suggested usages:
// 
// for:
//   sqlite3_pstmt ps = db.prepare(...);
//   
// to just execute:
//   ps.exec(a...);
// 
// to read first column of first row:
//   ps.exec<ColType>(a...);
//   
// to read Ith column of first row:
//   ps.exec<ColType, Ith>(a...);
//   
// to read N columns of first row:
//   ps.exec<TupleLike<ColTypes...>>(a...);
//
// to read N columns start at Ith of first row:
//   ps.exec<TupleLike<ColTypes...>, Ith>(a...);
//
// to read multiple rows
// {
//     // sqlite3_pstmt ps = db.prepare(...);
//     // or if sqlite3_pstmt itself is not scoped, use: 
//     auto resetter = ps.scoped_resetter();
// 
//     ps.args(...);
//     
//     while(ps.next())
//     {
//         get columns of current row use various retrieve methods
//     }
// }
class sqlite3_pstmt : public unique_handle<sqlite3_stmt*, finalize_sqlite3_pstmt>
{
    friend class sqlite3_conn;
    using handle_t::handle_t;

    bool _needReset = false;
    int _stepErr = 0;

public:
    sqlite3* db_handle() const noexcept { return sqlite3_db_handle(checked_handle()); }

    char const* sql() const noexcept { return sqlite3_sql(checked_handle()); }
    sqlite3_str expanded_sql() noexcept { return sqlite3_str(sqlite3_expanded_sql(checked_handle())); }

#ifdef SQLITE_ENABLE_NORMALIZE
    char const* normalized_sql() noexcept { return sqlite3_normalized_sql(checked_handle()); }
#endif

    bool is_explain            () const noexcept { return sqlite3_stmt_isexplain(checked_handle()) == 1; }
    bool is_explain_query_plain() const noexcept { return sqlite3_stmt_isexplain(checked_handle()) == 2; }

    sqlite3_errc change_to_explain(int m = 1)
    {
        reset_state();
        return static_cast<sqlite3_errc>(sqlite3_stmt_explain(checked_handle(), m));
    }

    sqlite3_errc change_to_explain_query_plain() { return change_to_explain(2); }    
    sqlite3_errc change_back_from_explain     () { return change_to_explain(0); }

    bool readonly() const noexcept { return sqlite3_stmt_readonly(checked_handle()); }
    bool busy    () const noexcept { return sqlite3_stmt_busy(checked_handle()); }

    // https://www.sqlite.org/c3ref/reset.html
    // https://www.sqlite.org/c3ref/step.html
    // 
    // reset to initial state, ready to be re-executed:
    // close any opened cursor;
    // bindings are NOT touched, but cached results may be destroyed.
    // 
    // sqlite3_reset() should be called between last sqlite3_step() and next sqlite3_bind_xxx()
    // 
    // NOTE: reset_state() is called automatically in xxx_arg()
    //       But to clean things up ASAP, reset_state() can also be called right after last next().
    //       scoped_resetter can be used for such purpose. And exec() already does that.
    void reset_state()
    {
        if(_needReset)
        {
            // If sqlite3_finalize()/sqlite3_reset() return an error,
            // it's either the same as last sqlite3_step() which was failed,
            // or SQLITE_BUSY if locking constraints prevent the database
            // change from committing.
            // The operation itself should always succeed.
            int err = sqlite3_reset(checked_handle());

            int stepErr = _stepErr;
            _stepErr =  0;
            _needReset = false;

            if(err && err != stepErr)
            {
                DSK_REPORTF("sqlite3_reset: %s", g_sqlite3_err_cat.message(err).c_str());
            }
        }
    }

    // reset_state() when goes out of scope;
    // NOTE: reset_state() is also called in xxx_arg()
    // 
    // usage:
    //    {
    //        auto resetter = stmt.scoped_resetter();
    //        stmt.args(...); // bind args
    //        while(stmt.next()) // execute and step cursor until last.
    //        {
    //            stmt.cols(...); // retrieve records
    //        }
    //        //
    //        stmt.args(...); // reset then bind args
    //        while(stmt.next())
    //            ...
    //    }
    template<bool Enable = true>
    auto scoped_resetter() noexcept
    {
        if constexpr(Enable)
        {
            struct scoped_resetter_t
            {
                sqlite3_pstmt& stmt;
                explicit scoped_resetter_t(sqlite3_pstmt& s) : stmt(s) {}
                ~scoped_resetter_t() { stmt.reset_state(); }
            };

            return scoped_resetter_t(*this);
        }
        else
        {
            return blank_c;
        }
    }


    // Execute the statement with args:
    //    If T is void, returns nothing.
    //    If T is _optional_ of _tuple_like_, the result maybe empty, in which case returns nullopt.
    //    Otherwise, returns first row via cols<Start>(t).
    // NOTE: If Reset = true, don't use non-ownership T like string_view,
    //       as the statement will be reset when this method returns.
    //       Assumes no gaps between arg indices, and first arg index is 1. (see: args(a...)).
    template<class T = void, size_t Start = 0, bool Reset = true>
    expected<T> exec(auto const&... a)
    {
        auto resetter = scoped_resetter<Reset>();

        DSK_E_TRY_ONLY(args(a...));

        if constexpr(_void_<T>)
        {
            DSK_E_TRY_ONLY(next());
            return {};
        }
        else
        {
            return next<T>(Start);
        }
    }

// argument binding
    
    // sqlite arg binding:
    // https://www.sqlite.org/lang_expr.html#varparam
    // 
    // an arg is bound to an index [1, SQLITE_MAX_VARIABLE_NUMBER=32766]
    // an index can be assigned a name.
    //
    // ?NNN           : integer     , index = NNN;
    // ?              : nameless    , index = largest assigned index to left + 1;
    // :AAA/@AAA/$AAA : alphanumeric, index = largest assigned index to left + 1;

    // return the largest assigned index
    int largest_index() const noexcept { return sqlite3_bind_parameter_count(checked_handle()); }
    
    // only works if no gaps between indices, and first index is 1;
    int arg_count() const noexcept { return largest_index(); }

    // find index of named arg. ?NNN/:AAA/@AAA/$AAA, include prefix '?:@$'
    int arg_index(_byte_str_ auto const& utf8name) const
    {
        return sqlite3_bind_parameter_index(checked_handle(), cstr_data<char>(as_cstr(utf8name)));
    }

    char const* arg_name(int i) const noexcept
    {
        DSK_ASSERT(i > 0);
        return sqlite3_bind_parameter_name(checked_handle(), i);
    }

    sqlite3_errc null_arg(int i)
    {
        DSK_ASSERT(i > 0);
        reset_state();
        return static_cast<sqlite3_errc>(sqlite3_bind_null(checked_handle(), i));
    }
    
    error_code num_arg(int i, std::integral auto v)
    {
        DSK_ASSERT(i > 0);
        reset_state();
        DSK_U_TRY_FWD(t, num_cast_u<int64_t>(v));
        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(sqlite3_bind_int64(checked_handle(), i, t)));
        return {};
    }

    error_code num_arg(int i, std::floating_point auto v)
    {
        DSK_ASSERT(i > 0);
        reset_state();
        DSK_U_TRY_FWD(t, num_cast_u<double>(v));
        DSK_E_TRY_ONLY(static_cast<sqlite3_errc>(sqlite3_bind_double(checked_handle(), i, t)));
        return {};
    }

    // NOTE: text and blob bindings should be alive until reset.
    sqlite3_errc blob_arg(int i, _borrowed_buf_ auto&& b)
    {
        DSK_ASSERT(i > 0);

        reset_state();

        auto* d = buf_data(b);
        auto  n = buf_byte_size<sqlite3_uint64>(b);

        if(! d) // if d is null, sqlite3_bind_blobXX() will treat it as NULL
        {
            d = reinterpret_cast<decltype(d)>(1);
            DSK_ASSERT(n == 0);
        }

        return static_cast<sqlite3_errc>(sqlite3_bind_blob64(checked_handle(), i, d, n, SQLITE_STATIC));
    }

    // NOTE: text and blob bindings should be alive until reset.
    sqlite3_errc text_arg(int i, _borrowed_str_ auto&& s, unsigned char encoding) requires(sizeof(str_val_t<decltype(s)>) <= 2)
    {
        DSK_ASSERT(i > 0);

        reset_state();

        auto* d = str_data<char>(s);
        auto  n = str_byte_size<sqlite3_uint64>(s);

        if(! d) // if d is null, sqlite3_bind_textXX() will treat it as NULL
        {
            d = reinterpret_cast<decltype(d)>(1);
            DSK_ASSERT(n == 0);
        }

        return static_cast<sqlite3_errc>(sqlite3_bind_text64(checked_handle(), i, d, n, SQLITE_STATIC, encoding));
    }

    sqlite3_errc utf8_arg(int i, _borrowed_str_ auto&& s) requires(sizeof(str_val_t<decltype(s)>) == 1)
    {
        return text_arg(i, s, SQLITE_UTF8);
    }

    sqlite3_errc utf16_arg(int i, _borrowed_str_ auto&& s) requires(sizeof(str_val_t<decltype(s)>) == 2)
    {
        return text_arg(i, s, SQLITE_UTF16);
    }

    sqlite3_errc ptr_arg(int i, void* p, char const* type, void(*dtor)(void*) = nullptr)
    {
        DSK_ASSERT(i > 0);
        reset_state();
        return static_cast<sqlite3_errc>(sqlite3_bind_pointer(checked_handle(), i, p, type, dtor));
    }

    sqlite3_errc ptr_arg(int i, sqlite3_ptr_arg const& a, void(*dtor)(void*) = nullptr)
    {
        return ptr_arg(i, a.p, a.t, dtor);
    }
    

    auto arg(int i, auto&& a)
    {
        DSK_ASSERT(i > 0);
        return bind_sqlite3_arg(*this, i, DSK_FORWARD(a));
    }
    
    auto arg(_byte_str_ auto const& n, auto&& a)
    {
        return arg(arg_index(n), DSK_FORWARD(a));
    }

    // assumes no gaps between indices, and first index is 1;
    auto args(auto&&... as)
    {
        DSK_ASSERT(arg_count() == sizeof...(as));

        return foreach_elms_until_err
        (
            fwd_as_tuple(DSK_FORWARD(as)...),
            [&]<auto I>(auto&& a) -> error_code { return arg(I + 1, DSK_FORWARD(a)); }
        );
    }

    // sqlite3_clear_bindings() is just bad and ambigous design, don't use it.
    // // bind all arguments to NULL, also release SQLITE_TRANSIENT bindings
    // sqlite3_errc bind_all_args_to_null()
    // {
    //     return static_cast<sqlite3_errc>(sqlite3_clear_bindings(checked_handle()));
    // }


/// result retrieve

    // actually execute the statement on first call, args should be all binded properly
    template<size_t Start = 0>
    expected<bool> next(auto&... vs)
    {
        int r = sqlite3_step(checked_handle());

        _needReset = true;

        if(r == SQLITE_ROW)
        {
            if constexpr(sizeof...(vs))
            {
                DSK_E_TRY_ONLY(cols<Start>(vs...));
            }

            return true;
        }
        
        if(r == SQLITE_DONE)
        {
            return false;
        }

        _stepErr = r;
        return static_cast<sqlite3_errc>(r);
    }

    template<class T>
    expected<T> next(int start = 0)
    {
        DSK_E_TRY(bool hasRow, next());

        //if constexpr(requires{ requires _optional_<T> && _tuple_like_<val_t<T>>;}) clang ICE
        if constexpr(DSK_CONDITIONAL_V(_optional_<T>, _tuple_like_<val_t<T>>, false))
        {
            if(hasRow)
            {
                typename T::value_type v;
                DSK_E_TRY_ONLY(get_cols(start, v));
                return v;
            }

            return nullopt;
        }
        else
        {
            if(hasRow) return cols<T>(start);
            else       return sqlite3_general_errc::no_more_row;
        }
    }

    // NOTE: column is 0 based

    int col_cnt() const noexcept { return sqlite3_column_count(checked_handle()); }
    int col_type(int i) const noexcept { return sqlite3_column_type(checked_handle(), i); }

    bool is_null(int i) const noexcept { return col_type(i) == SQLITE_NULL; }

    // only for SELECT statement
    char const*     col_name(int i) const noexcept { return sqlite3_column_name(checked_handle(), i); }
    char const* col_ori_name(int i) const noexcept { return sqlite3_column_origin_name(checked_handle(), i); }
    char const* col_tbl_name(int i) const noexcept { return sqlite3_column_table_name(checked_handle(), i); }
    char const*  col_db_name(int i) const noexcept { return sqlite3_column_database_name(checked_handle(), i); }


    error_code get_num(int i, _arithmetic_ auto& v) noexcept
    {
        switch(col_type(i))
        {
            case SQLITE_INTEGER: return cvt_num(sqlite3_column_int64 (checked_handle(), i), v);
            case SQLITE_FLOAT  : return cvt_num(sqlite3_column_double(checked_handle(), i), v);
        }

        return sqlite3_errc::mismatch;

        // sqlite3_column_int() returns the lower 32 bits of that value without checking for overflows,
        // so it is not used here.
        // sqlite will try its best to convert text/blob to numerics,
        // which cannot detect the conversion failure.
        // it's better to leave that to the user.
    }
 
    template<_arithmetic_ T>
    expected<T> num(int i = 0) noexcept
    {
        T v;
        DSK_E_TRY_ONLY(get_num(i, v));
        return v;
    }

    auto   bool_(int i = 0) noexcept { return num<bool              >(i); }
    auto  short_(int i = 0) noexcept { return num<short             >(i); }
    auto ushort_(int i = 0) noexcept { return num<unsigned short    >(i); }
    auto    int_(int i = 0) noexcept { return num<int               >(i); }
    auto   uint_(int i = 0) noexcept { return num<unsigned          >(i); }
    auto   long_(int i = 0) noexcept { return num<long              >(i); }
    auto  ulong_(int i = 0) noexcept { return num<unsigned long     >(i); }
    auto  llong_(int i = 0) noexcept { return num<long long         >(i); }
    auto ullong_(int i = 0) noexcept { return num<unsigned long long>(i); }
    auto  float_(int i = 0) noexcept { return num<float             >(i); }
    auto double_(int i = 0) noexcept { return num<double            >(i); }

    auto   int8_(int i = 0) noexcept { return num<int8_t  >(i); }
    auto  uint8_(int i = 0) noexcept { return num<uint8_t >(i); }
    auto  int16_(int i = 0) noexcept { return num<int16_t >(i); }
    auto uint16_(int i = 0) noexcept { return num<uint16_t>(i); }
    auto  int32_(int i = 0) noexcept { return num<int32_t >(i); }
    auto uint32_(int i = 0) noexcept { return num<uint32_t>(i); }
    auto  int64_(int i = 0) noexcept { return num<int64_t >(i); }
    auto uint64_(int i = 0) noexcept { return num<uint64_t>(i); }
    auto  sizet_(int i = 0) noexcept { return num<size_t  >(i); }


    // NOTE:
    // be careful with non-ownership type like string_view, it only lasts until the statement is reset or destructed;
    // https://en.cppreference.com/w/cpp/language/lifetime
    // All temporary objects are destroyed as the last step in evaluating the full-expression
    // that (lexically) contains the point where they were created ...

    // extract blob or text with same encoding directly, transcoding text if necessary
    // stringify other types.

    sqlite3_errc get_text(int i, _buf_ auto& v)
    {
        using ch_t = buf_val_t<decltype(v)>;

        void const* s = nullptr;
        int n = 0;

        if constexpr(sizeof(ch_t) == 1)
        {
            s = sqlite3_column_text(checked_handle(), i); // must before sqlite3_column_bytes
            n = sqlite3_column_bytes(checked_handle(), i);
        }
        else if constexpr(sizeof(ch_t) == 2)
        {
            s = sqlite3_column_text16(checked_handle(), i); // must before sqlite3_column_bytes16
            n = sqlite3_column_bytes16(checked_handle(), i);
        }
        else
        {
            static_assert(false, "unsupported type");
        }

        // sqlite3_column_textXX() only returns nullptr for NULL.
        if(! s && SQLITE_NOMEM == sqlite3_extended_errcode(db_handle()))
        {
            return static_cast<sqlite3_errc>(SQLITE_NOMEM);
        }

        DSK_ASSERT(n % sizeof(ch_t) == 0);

        assign_buf(v, static_cast<ch_t const*>(s), static_cast<size_t>(n) / sizeof(ch_t));
        return {};
    }

    template<_buf_ B>
    expected<B, sqlite3_errc> text(int i = 0)
    {
        B v;
        DSK_E_TRY_ONLY(get_text(i, v));
        return v;
    }

    template<_buf_ B = std::string_view>
    B utf8(int i = 0)
    {
        static_assert(sizeof(buf_val_t<B>) == 1);
        return text<B>(i);
    }

    // UTF-16 native
    template<_buf_ B = std::u16string_view>
    B utf16(int i = 0)
    {
        static_assert(sizeof(buf_val_t<B>) == 2);
        return text<B>(i);
    }


    // extract text/blob directly, and stringify other type
    sqlite3_errc get_blob(int i, _buf_ auto& v)
    {
        auto* b = sqlite3_column_blob(checked_handle(), i); // must before sqlite3_column_bytes16
        auto  n = sqlite3_column_bytes(checked_handle(), i);

        // sqlite3_column_blob() only returns nullptr for NULL or zero length blob/text.
        if(! b && SQLITE_NOMEM == sqlite3_extended_errcode(db_handle()))
        {
            return static_cast<sqlite3_errc>(SQLITE_NOMEM);
        }

        using elem_t = buf_val_t<decltype(v)>;

        DSK_ASSERT(n % sizeof(elem_t) == 0);

        assign_buf(v, static_cast<elem_t const*>(b), static_cast<size_t>(n) / sizeof(elem_t));
        return {};
    }

    template<_buf_ B = std::span<std::byte const>>
    expected<B, sqlite3_errc> blob(int i = 0)
    {
        B v;
        DSK_E_TRY_ONLY(get_blob(i, v));
        return v;
    }


    auto get_col(int i, auto& v)
    {
        return get_sqlite3_col(*this, i, v);
    }

    template<class T>
    auto col(int i) -> expected<T, decltype(get_col(i, std::declval<T&>()))>
    {
        T v;
        DSK_E_TRY_ONLY(get_col(i, v));
        return v;
    }


    error_code get_cols(int start, _tuple_like_ auto&& vs)
    {
        DSK_ASSERT(start + elm_count<decltype(vs)> <= static_cast<size_t>(col_cnt()));

        return foreach_elms_until_err(vs, [&]<auto I>(auto&& v) -> error_code
        {
            if constexpr(_ignore_<decltype(v)>) return {};
            else                                return get_col(start + I, v);
        });
    }

    auto get_cols(int start, auto&... vs)
    {
        return get_cols(start, fwd_as_tuple(vs...));
    }

    template<size_t Start = 0>
    auto cols(auto&&... vs)
    {
        return get_cols(Start, DSK_FORWARD(vs)...);
    }

    template<class T>
    expected<T> cols(int start = 0)
    {
        T vs;
        DSK_E_TRY_ONLY(get_cols(start, vs));
        return vs;
    }

/// status
// https://sqlite.org/c3ref/c_stmtstatus_counter.html

    int status(int op, bool reset = false) noexcept { return sqlite3_stmt_status(checked_handle(), op, reset); }
    int reset_status(int op, bool reset = true) noexcept { return status(op, reset); }

    int fullscan_step_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_FULLSCAN_STEP, reset); }
    int reset_fullscan_step_cnt(bool reset = true) noexcept { return fullscan_step_cnt(reset); }

    int sort_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_SORT, reset); }
    int reset_sort_cnt(bool reset = true) noexcept { return sort_cnt(reset); }

    int autoindx_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_AUTOINDEX, reset); }
    int reset_autoindx_cnt(bool reset = true) noexcept { return autoindx_cnt(reset); }

    int vm_step_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_VM_STEP, reset); }
    int reset_vm_step_cnt(bool reset = true) noexcept { return vm_step_cnt(reset); }

    int prepare_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_REPREPARE, reset); }
    int reset_prepare_cnt(bool reset = true) noexcept { return prepare_cnt(reset); }

    int run_cnt(bool reset = false) noexcept { return status(SQLITE_STMTSTATUS_RUN, reset); }
    int reset_run_cnt(bool reset = true) noexcept { return run_cnt(reset); }

    int approximate_memused() noexcept { return status(SQLITE_STMTSTATUS_MEMUSED); }
};


inline auto dsk_bind_sqlite3_arg(bind_sqlite3_arg_cpo, sqlite3_pstmt& s, int i, sqlite3_ptr_arg const& a)
{
    return s.ptr_arg(i, a);
}

inline auto dsk_bind_sqlite3_arg(bind_sqlite3_arg_cpo, sqlite3_pstmt& s, int i, std::nullptr_t)
{
    return s.null_arg(i);
}

auto dsk_bind_sqlite3_arg(bind_sqlite3_arg_cpo, sqlite3_pstmt& s, int i, _arithmetic_ auto a)
{
    return s.num_arg(i, a);
}

// NOTE: any _buf_ of _char_t_ is _str_.
//       Use as_buf_of<non _char_t_>() or as_str_of<_char_t_>() to change default behaviour.
auto dsk_bind_sqlite3_arg(bind_sqlite3_arg_cpo, sqlite3_pstmt& s, int i, auto&& a)
requires(
    _borrowed_str_<decltype(a)> || _borrowed_buf_<decltype(a)>)
{
    if constexpr(_borrowed_str_<decltype(a)>)
    {
        if constexpr(sizeof(str_val_t<decltype(a)>) == 1)
        {
            return s.utf8_arg(i, a);
        }
        else if constexpr(sizeof(str_val_t<decltype(a)>) == 2)
        {
            return s.utf16_arg(i, a);
        }
        else
        {
            static_assert(false, "unsupported type");
        }
    }
    else
    {
        return s.blob_arg(i, a);
    }
}

auto dsk_bind_sqlite3_arg(bind_sqlite3_arg_cpo, sqlite3_pstmt& s, int i, _optional_ auto&& a)
->
    decltype(bind_sqlite3_arg(s, i, *DSK_FORWARD(a)))
{
    if(a) return bind_sqlite3_arg(s, i, *DSK_FORWARD(a));
    else  return s.null_arg(i);
}


template<_arithmetic_ T>
auto dsk_get_sqlite3_col(get_sqlite3_col_cpo, sqlite3_pstmt& s, int i, T& v)
{
    return s.get_num(i, v);
}

auto dsk_get_sqlite3_col(get_sqlite3_col_cpo, sqlite3_pstmt& s, int i, _buf_ auto& v)
{
    // NOTE: if 'NOT NULL' is not specified on column, NULL may return.
    //       Use optional<> for nullable column.
    switch(s.col_type(i))
    {
        case SQLITE_TEXT: return s.get_text(i, v);
        case SQLITE_BLOB: return s.get_blob(i, v);
        default:
            return sqlite3_errc::mismatch;
    }
}

auto dsk_get_sqlite3_col(get_sqlite3_col_cpo, sqlite3_pstmt& s, int i, _optional_ auto& v)
->
    decltype(s.get_col(i, assure_val(v)))
{
    if(s.is_null(i)) { v.reset(); return {}; }
    else             { return s.get_col(i, assure_val(v)); }
}


} // namespace std
