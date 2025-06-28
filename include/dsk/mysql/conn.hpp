#pragma once

#include <dsk/task.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <dsk/mysql/fields.hpp>
#include <boost/mysql/any_connection.hpp>


namespace dsk{


using mysql_conn_opts      = mysql::any_connection_params;
using mysql_connect_params = mysql::connect_params;
using mysql_addr           = mysql::any_address;
using mysql_unix_path      = mysql::unix_path;
using mysql_host_and_port  = mysql::host_and_port;
using mysql_addr_type      = mysql::address_type;
using mysql_ssl_mode       = mysql::ssl_mode;
using mysql_charset        = mysql::character_set;
using mysql_stmt           = mysql::statement;
using mysql_results        = mysql::results;
using mysql_exec_state     = mysql::execution_state;
using mysql_pipeline_req   = mysql::pipeline_request;
using mysql_stage_response = mysql::stage_response;

template<class... Rows>
using mysql_static_results = mysql::static_results<Rows...>;

template<class... Rows>
using mysql_static_exec_state = mysql::static_execution_state<Rows...>;


template<class T>
concept _mysql_results_ = _no_cvref_same_as_<T, mysql_results>
                          || _specialized_from_<T, mysql_static_results>;
template<class T>
concept _mysql_exec_state_ = _no_cvref_same_as_<T, mysql_exec_state>
                             || _specialized_from_<T, mysql_static_exec_state>;


auto format_sql(mysql::format_options opts, _byte_str_ auto const& format, auto&&... args)
{
    std::initializer_list<mysql::format_arg> argList{{mysql::string_view(), DSK_FORWARD(args)}...};

    mysql::basic_format_context<string> ctx(opts);

    mysql::detail::format_state(ctx, argList).format(str_view<char>(format));

    return std::move(ctx).get().value();
}


class mysql_conn : public mysql::any_connection
{
    using base = mysql::any_connection;

public:
    using base::base;

    mysql_conn(base&& b)
        : base(mut_move(b))
    {}

    explicit mysql_conn(mysql_conn_opts const& params = {})
        : base(DSK_DEFAULT_IO_CONTEXT, params)
    {}

    auto connect(mysql_connect_params const& params, auto&... diag)
    {
        return base::async_connect(params, diag..., use_async_op);
    }

    auto execute(auto&& req, _mysql_results_ auto& results, auto&... diag)
    {
        return base::async_execute(DSK_FORWARD(req), results, diag..., use_async_op);
    }

    auto start_execution(auto&& req, _mysql_exec_state_ auto& st, auto&... diag)
    {
        return base::async_start_execution(DSK_FORWARD(req), st, diag..., use_async_op);
    }

    auto read_some_rows(mysql_exec_state& st, auto&... diag)
    {
        return base::async_read_some_rows(st, diag..., use_async_op);
    }

    template<class SpanElem, class... StaticRows>
    auto read_some_rows(mysql_static_exec_state<StaticRows...>& st, boost::span<SpanElem> const& output, auto&... diag)
    {
        return base::async_read_some_rows(st, output, diag..., use_async_op);
    }

    auto read_resultset_head(auto& st, auto&... diag)
    {
        return base::async_read_resultset_head(st, diag..., use_async_op);
    }

    auto prepare_statement(_borrowed_byte_str_ auto&& stmt, auto&... diag)
    {
        return base::async_prepare_statement(str_view<char>(stmt), diag..., use_async_op);
    }

    auto close_statement(mysql_stmt const& stmt, auto&... diag)
    {
        return base::async_close_statement(stmt, diag..., use_async_op);
    }

    auto set_character_set(mysql_charset const& charset, auto&... diag)
    {
        return base::async_read_resultset_head(charset, diag..., use_async_op);
    }

    auto ping(auto&... diag)
    {
        return base::async_ping(diag..., use_async_op);
    }

    auto reset_connection(auto&... diag)
    {
        return base::async_reset_connection(diag..., use_async_op);
    }

    auto close(auto&... diag)
    {
        return base::async_close(diag..., use_async_op);
    }

    auto run_pipeline(mysql_pipeline_req const& req, std::vector<mysql_stage_response>& res, auto&... diag)
    {
        return base::async_run_pipeline(req, res, diag..., use_async_op);
    }


    /// extended methods

    decltype(auto) to_request(auto&& query, auto&&... args)
    {
        if constexpr(_byte_str_<decltype(query)>)
        {
            return format_sql(format_opts().value(), query, DSK_FORWARD(args)...);
        }
        else if constexpr(_derived_from_<decltype(query), mysql_stmt>)
        {
            return query.bind(DSK_FORWARD(args)...);
        }
        else
        {
            static_assert(sizeof...(args) == 0);
            return DSK_FORWARD(query);
        }
    }

    auto exec_get(_mysql_results_ auto& r, auto&& query, auto&&... args)
    {
        return execute(to_request(DSK_FORWARD(query), DSK_FORWARD(args)...), r);
    }

    // Get the result at Set'th set, Row'th row, start at StartCol'th column.
    // Negative Set and Row start at end.
    // If r is _optional_ of _tuple_like_, the row may not exist, in which case r is set to nullopt.
    // Otherwise, r can be any type supporting get_fields(row, r)
    template<size_t StartCol = 0, int64_t Row = 0, int64_t Set = -1>
    task<> exec_get(auto& r, auto&& query, auto&&... args) requires(! _mysql_results_<decltype(r)>)
    {
        auto&& req = to_request(DSK_FORWARD(query), DSK_FORWARD(args)...);

        return [](mysql_conn* self, auto& r, DSK_LREF_OR_VAL_T(req) req) -> task<>
        {
            mysql_results results;
            DSK_TRY self->execute(std::move(req), results);

            int64_t nSet = static_cast<int64_t>(results.size());
            int64_t iSet = (Set >= 0 ? Set : nSet + Set);

            DSK_ASSERT(0 <= iSet && iSet < nSet);

            auto&& rows = results[iSet].rows();

            int64_t nRow = static_cast<int64_t>(rows.size());
            int64_t iRow = (Row >= 0 ? Row : nRow + Row);

            //if constexpr(requires{ requires _optional_<decltype(r)> && _tuple_like_<decltype(*r)>; }) clang ICE
            if constexpr(DSK_CONDITIONAL_V(_optional_<decltype(r)>, _tuple_like_<decltype(*r)>, false))
            {
                if(0 <= iRow && iRow < nRow)
                {
                    DSK_TRY_SYNC get_fields<StartCol>(rows[iRow], assure_val(r));
                }
                else
                {
                    r.reset();
                }
            }
            else
            {
                if(0 <= iRow && iRow < nRow)
                {
                    DSK_TRY_SYNC get_fields<StartCol>(rows[iRow], r);
                }
                else
                {
                    DSK_THROW(errc::out_of_bound);
                }
            }

            DSK_RETURN();
        }
        (this, r, DSK_FORWARD(req));
    }

    template<class R = void, size_t StartCol = 0, int64_t Row = 0, int64_t Set = -1>
    task<R> exec(auto&& query, auto&&... args)
    {
        auto&& req = to_request(DSK_FORWARD(query), DSK_FORWARD(args)...);

        return [](mysql_conn* self, DSK_LREF_OR_VAL_T(req) req) -> task<R>
        {
            if constexpr(_void_<R>)
            {
                mysql_results r;
                DSK_TRY self->execute(std::move(req), r);
                DSK_RETURN();
            }
            else
            {
                R r;
                DSK_TRY self->exec_get<StartCol, Row, Set>(r, std::move(req));
                DSK_RETURN_MOVED(r);
            }
        }
        (this, DSK_FORWARD(req));
    }

    auto start_exec(_mysql_exec_state_ auto& st, auto&& query, auto&&... args)
    {
        return start_execution(to_request(DSK_FORWARD(query), DSK_FORWARD(args)...), st);
    }

    template<class Alloc>
    class transaction_t
    {
        struct data_t
        {
            mysql_conn& conn;
            bool done = false;
        };

        allocated_unique_ptr<data_t, Alloc> _d;

    public:
        transaction_t(transaction_t&&) = default;

        explicit transaction_t(mysql_conn& conn) noexcept
            : _d(allocate_unique<data_t>(Alloc(), conn))
        {}

        ~transaction_t()
        {
            DSK_ASSERT(! _d || _d->done);
        }

        constexpr auto operator<=>(transaction_t const&) const = default;

        task<> commit()
        {
            return [](data_t* d) -> task<>
            {
                DSK_ASSERT(! d->done);

                DSK_TRY d->conn.exec("COMMIT");
                d->done = true;
            
                DSK_RETURN();
            }
            (_d.get());
        }

        task<> rollback()
        {
            return [](data_t* d) -> task<>
            {
                DSK_ASSERT(! d->done);

                DSK_TRY d->conn.exec("ROLLBACK");
                d->done = true;

                DSK_RETURN();
            }
            (_d.get());
        }

        task<> rollback_if_uncommited()
        {
            return [](data_t* d) -> task<>
            {
                if(! d->done)
                {
                    DSK_TRY d->conn.exec("ROLLBACK");
                    d->done = true;
                }

                DSK_RETURN();
            }
            (_d.get());
        }
    };

    template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
    task<transaction_t<Alloc>> make_transaction(_byte_str_ auto... opt)
    {
        static_assert(sizeof...(opt) <= 1);

        auto&& req = DSK_CONDITIONAL_V(sizeof...(opt), (cat_str("START TRANSACTION ", opt...))
                                                     , "START TRANSACTION");
        DSK_TRY exec(DSK_FORWARD(req));

        transaction_t<Alloc> trxn(*this);

        DSK_WAIT add_parent_cleanup(trxn.rollback_if_uncommited());

        DSK_RETURN_MOVED(trxn);
    }
};


} // namespace dsk
