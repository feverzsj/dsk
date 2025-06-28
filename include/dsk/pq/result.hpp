#pragma once

#include <dsk/util/handle.hpp>
#include <dsk/pq/err.hpp>
#include <dsk/pq/data_types.hpp>


namespace dsk{


class pq_str : public unique_handle<char*, PQfreemem>
{
    using base = unique_handle<char*, PQfreemem>;

public:
    using base::base;

    char* c_str() const noexcept { return handle(); }
    char*  data() const noexcept { return handle(); }
    size_t size() const noexcept { return strlen(handle()); }
};

static_assert(_byte_buf_<pq_str>);
static_assert(_byte_cstr_<pq_str>);


class pq_result : public unique_handle<PGresult*, PQclear>
{
    using base = unique_handle<PGresult*, PQclear>;

public:
    using base::base;

    static char const* status_str(ExecStatusType s) noexcept { return PQresStatus(s); }

    ExecStatusType  status() const noexcept { return PQresultStatus(checked_handle()); }
    char const* status_str() const noexcept { return status_str(status()); }
    
    char const* err_fld(int fld) const noexcept { return PQresultErrorField(checked_handle(), fld); }
    char const* err_msg(       ) const noexcept { return PQresultErrorMessage(checked_handle()); }

    pq_str err_msg(PGVerbosity verbosity, PGContextVisibility visibility)
    {
        return pq_str(PQresultVerboseErrorMessage(checked_handle(), verbosity, visibility));
    }

    error_code err() const noexcept
    {
        switch(status())
        {
            case PGRES_COMMAND_OK:   // command that doesn't return any row
            case PGRES_TUPLES_OK:    // common result
            case PGRES_SINGLE_TUPLE: // single row mode
            //case PGRES_TUPLES_CHUNK: // chunked row mode TODO: waiting for vcpkg libpq update
            case PGRES_PIPELINE_SYNC:
            case PGRES_PIPELINE_ABORTED:
            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
            case PGRES_COPY_BOTH:
                return {};
            case PGRES_FATAL_ERROR:
                if(char const* f = err_fld(PG_DIAG_SQLSTATE))
                {
                    DSK_E_TRY_FWD(e, str_to<int>(f, 36));
                    return static_cast<pq_sql_errc>(e);
                }
                else
                {
                    return pq_errc::result_fatal_error;
                }
            case PGRES_EMPTY_QUERY     : return pq_errc::result_empty_query;
            case PGRES_BAD_RESPONSE    : return pq_errc::result_bad_response;
            case PGRES_NONFATAL_ERROR  : return pq_errc::result_nonfatal_error;
        }

        return pq_errc::result_unknown_error;
    }

    // return 0 for empty/nullptr result
    int row_cnt() const noexcept { return PQntuples(/*checked_*/handle()); }
    int col_cnt() const noexcept { return PQnfields(/*checked_*/handle()); }
    int fld_cnt() const noexcept { return col_cnt(); }
    
    char const* fld_name(int idx) const noexcept { return PQfname(checked_handle(), idx); }
    
    int fld_idx(_byte_str_ auto const& name) const noexcept
    {
        return PQfnumber(checked_handle(), cstr_data<char>(as_cstr(name)));
    }

    // Returns the space allocated server side for this column in a database row.
    // A negative value indicates the data type is variable-length.
    int fld_alloc_size(int idx) const noexcept { return PQfsize(checked_handle(), idx); }

    int fld_type_modifier(int idx) const noexcept { return PQfmod (checked_handle(), idx); }

    pq_oid_e fld_oid(int idx) const noexcept { return static_cast<pq_oid_e>(PQftype  (checked_handle(), idx)); }
    pq_fmt_e fld_fmt(int idx) const noexcept { return static_cast<pq_fmt_e>(PQfformat(checked_handle(), idx)); }

    int fld_tbl_col(int idx) const noexcept { return PQftablecol(checked_handle(), idx); }
    Oid fld_tbl_oid(int idx) const noexcept { return PQftable   (checked_handle(), idx); }

    Oid inserted_row_oid() const noexcept { return PQoidValue(checked_handle()); }

    bool is_null(int row, int col) const noexcept { return PQgetisnull(checked_handle(), row, col); }

    // for COPY
    bool is_binary_copy() const noexcept { return PQbinaryTuples(checked_handle()); }

    char const*       cmd_status_str() const noexcept { return PQcmdStatus(checked_handle()); }
    char const* affected_row_cnt_str() const noexcept { return PQcmdTuples(checked_handle()); }

    // only useful when inspecting the result of PQdescribePrepared/PQsendDescribePrepared
    int      param_cnt(       ) const noexcept { return PQnparams(checked_handle()); }
    pq_oid_e param_oid(int idx) const noexcept { return static_cast<pq_oid_e>(PQparamtype(checked_handle(), idx)); }

    // value access

    char const* data(int row, int col) const noexcept { return PQgetvalue(checked_handle(), row, col); }
    int         size(int row, int col) const noexcept { return PQgetlength(checked_handle(), row, col); }
    

    error_code get_val(int row, int col, auto& v) const
    {
        DSK_ASSERT(row < row_cnt());
        DSK_ASSERT(col < col_cnt());

        if(is_null(row, col))
        {
            return pq_errc::null_for_non_optional_type;
        }

        auto fmt = fld_fmt(col);
        auto ft  = fld_oid(col);
        auto d   = data(row, col);
        auto n   = size(row, col);

        if(fmt == pq_fmt_text || is_textual(ft))
        {
            if constexpr(requires{ from_pq_textual(d, n, v); })
            {
                DSK_U_TRY_ONLY(from_pq_textual(d, n, v));
            }
            else
            {
                return pq_errc::unsupported_textual_conv;
            }
        }
        else
        {
            DSK_ASSERT(fmt == pq_fmt_binary);

            if constexpr(requires{ from_pq_binary(ft, d, n, v); })
            {
                DSK_U_TRY_ONLY(from_pq_binary(ft, d, n, v));
            }
            else
            {
                return pq_errc::unsupported_binary_conv;
            }
        }

        return {};
    }


    error_code get_val(int row, int col, _optional_ auto& v) const
    {
        if(is_null(row, col))
        {
            v.reset();
            return{};
        }

        return get_val(row, col, assure_val(v));
    }

    template<class T>
    expected<T> val(int row, int col) const
    {
        T v;
        DSK_E_TRY_ONLY(get_val(row, col, v));
        return v;
    }


    error_code get_vals(int row, int startCol, _tuple_like_ auto&& vs) const
    {
        DSK_ASSERT(row < row_cnt());
        DSK_ASSERT(static_cast<int>(startCol + elm_count<decltype(vs)>) <= col_cnt());

        return foreach_elms_until_err(vs, [&]<auto I>(auto&& v)
        {
            if constexpr(_ignore_<decltype(v)>) return error_code();
            else                                return get_val(row, startCol + I, v);
        });
    }

    error_code get_vals(int row, int startCol, auto&... vs) const
    {
        return get_vals(row, startCol, fwd_as_tuple(vs...));
    }

    error_code vals(int row, int startCol, auto&&... vs) const
    {
        return get_vals(row, startCol, DSK_FORWARD(vs)...);
    }

    template<class T>
    expected<T> vals(int row, int startCol) const
    {
        T v;
        DSK_E_TRY_ONLY(get_vals(row, startCol, v));
        return v;
    }


    template<size_t StartCol = 0>
    error_code get_cols(int row, auto&&... vs) const
    {
        return get_vals(row, StartCol, DSK_FORWARD(vs)...);
    }

    template<size_t StartCol = 0>
    error_code cols(int row, auto&&... vs) const
    {
        return vals(row, StartCol, DSK_FORWARD(vs)...);
    }

    template<class T, size_t StartCol = 0>
    expected<T> cols(int row) const
    {
        return vals<T>(row, StartCol);
    }

    struct row_ref
    {
        pq_result const* res = nullptr;
        int row = 0;

        char const* data(int i) const noexcept { return res->data(row, i); }
        int         size(int i) const noexcept { return res->size(row, i); }
        bool     is_null(int i) const noexcept { return res->is_null(row, i); }

        error_code get_col(int i, auto& v) const
        {
            return res->get_val(row, i, v);
        }

        template<class T>
        expected<T> col(int i) const
        {
            return res->val<T>(row, i);
        }

        error_code get_cols(int start, auto&&... vs) const
        {
            return res->get_vals(row, start, DSK_FORWARD(vs)...);
        }

        template<size_t Start = 0>
        error_code cols(auto&&... vs) const
        {
            return res->vals(row, Start, DSK_FORWARD(vs)...);
        }

        template<class T>
        expected<T> cols(int start = 0) const
        {
            return res->vals<T>(row, start);
        }
    };

    struct row_ptr
    {
        row_ref r;
        row_ref* operator->() noexcept { return &r; }
    };

    struct iterator : protected row_ref
    {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = row_ref;
        using difference_type   = ptrdiff_t;
        using pointer           = row_ptr; // value type as the iterator may be modified, after returning from explicit call of operator->().
        using reference         = row_ref; // value type as the iterator may be modified, after returning the dereferenced type.

        iterator(pq_result const* res, int row) noexcept
            : row_ref(res, row)
        {}

        row_ref operator* () const noexcept { return  *this ; }
        row_ptr operator->() const noexcept { return {*this}; }

        iterator& operator++() noexcept { ++row; return *this; }
        iterator& operator--() noexcept { --row; return *this; }

        iterator operator++(int) noexcept { auto t = *this; ++row; return t; }
        iterator operator--(int) noexcept { auto t = *this; --row; return t; }

        bool operator==(iterator const& r) const noexcept
        {
            DSK_ASSERT(res == r.res);
            return row == r.row;
        }
    };

    iterator begin() const noexcept { return {this,         0}; }
    iterator   end() const noexcept { return {this, row_cnt()}; }
};


// single row result
class pq_row_result : public pq_result
{
public:
    explicit pq_row_result(pq_result&& r) noexcept
        : pq_result(mut_move(r))
    {
        DSK_ASSERT(! valid() || status() == PGRES_SINGLE_TUPLE);
    }

    char const* data(int i) const noexcept { return pq_result::data(0, i); }
    int         size(int i) const noexcept { return pq_result::size(0, i); }
    bool     is_null(int i) const noexcept { return pq_result::is_null(0, i); }

    error_code get_col(int i, auto& v) const
    {
        return pq_result::get_val(0, i, v);
    }

    template<class T>
    expected<T> col(int i) const
    {
        return pq_result::val<T>(0, i);
    }

    error_code get_cols(int start, auto&&... vs) const
    {
        return pq_result::get_vals(0, start, DSK_FORWARD(vs)...);
    }

    template<size_t Start = 0>
    error_code cols(auto&&... vs) const
    {
        return pq_result::vals(0, Start, DSK_FORWARD(vs)...);
    }

    template<class T>
    expected<T> cols(int start = 0) const
    {
        return pq_result::vals<T>(0, start);
    }
};


} // namespace dsk
