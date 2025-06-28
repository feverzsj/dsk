#pragma once


#include <dsk/task.hpp>
#include <dsk/optional.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/string.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/util/allocate_unique.hpp>
#include <dsk/asio/tcp.hpp>
#include <dsk/pq/err.hpp>
#include <dsk/pq/result.hpp>
#include <dsk/pq/data_types.hpp>
#include <libpq-fe.h>


namespace dsk{


enum pq_result_e
{
    pq_none  = -1,
    pq_all   =  0,
    pq_row   =  1,
    pq_batch =  2,
};

template<
    pq_result_e E,
    class T = void
>
using pq_result_t = std::conditional_t<   (E <  0), void,
                    std::conditional_t<! _void_<T>, T,
                    std::conditional_t<   (E == 1), pq_row_result,
                                                    pq_result>>>;


struct pq_preapred : string
{
    using string::string;
};

template<class T>
concept _pq_preapred_ = _no_cvref_same_as_<T, pq_preapred>;


class pq_buf : public unique_handle<void*, PQfreemem>
{
    using base = unique_handle<void*, PQfreemem>;

    size_t _s = 0;

public:
    pq_buf(void* d, size_t s)
        : base(d), _s(s)
    {}

    template<_byte_ T = char>
    T*     data() const noexcept { return static_cast<T*>(handle()); }
    size_t size() const noexcept { return _s; }
};

static_assert(_byte_buf_<pq_buf>);
static_assert(_byte_str_<pq_buf>);


class pq_notify : public unique_handle<PGnotify*, PQfreemem>
{
    using base = unique_handle<PGnotify*, PQfreemem>;

public:
    using base::base;

    char const* relname() const noexcept { return checked_handle()->relname; }
    int			be_pid () const noexcept { return checked_handle()->be_pid ; }
    char const* extra  () const noexcept { return checked_handle()->extra  ; }
};


// thread safety:
// https://www.postgresql.org/docs/current/static/libpq-threading.html


// TODO: PGcancelConn  waiting for vcpkg libpq update
class pq_conn : public unique_handle<PGconn*, PQfinish>
{
    tcp_socket _skt;

    template<auto Poll>
    task<> do_connect(auto&& init, auto&&... args)
    {
        return [](pq_conn* self, DSK_LREF_OR_VAL_T(init) init, DSK_LREF_OR_VAL_T(args)... args) mutable -> task<>
        {
            DSK_TRY_SYNC init(DSK_MOVE_NON_LREF(args)...);

            tcp_socket skt;

            for(tcp_socket::wait_type wt = tcp_socket::wait_type::wait_write;;)
            {
                // socket may change across Poll() calls, so we must assign/release here.
                DSK_TRY_SYNC skt.assign(PQsocket(self->handle()));
                DSK_TRY skt.wait(wt);
                DSK_TRY_SYNC skt.release();

                auto r = Poll(self->handle());

                     if(r == PGRES_POLLING_OK     ) break;
                else if(r == PGRES_POLLING_READING) wt = tcp_socket::wait_type::wait_read;
                else if(r == PGRES_POLLING_WRITING) wt = tcp_socket::wait_type::wait_write;
                else if(r == PGRES_POLLING_FAILED ) DSK_THROW(pq_errc::connect_failed);
                else                                DSK_THROW(errc::unknown_exception);
            }

            DSK_TRY_SYNC self->_skt.release();
            DSK_TRY_SYNC self->_skt.assign(PQsocket(self->handle()));
            DSK_RETURN();
        }(
            this, DSK_FORWARD(init), DSK_FORWARD(args)...
        );
    }

    template<pq_result_e E, class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<E, T>> do_send(auto&& init, auto&&... args)
    {
        return [](pq_conn* self, DSK_LREF_OR_VAL_T(init) init, DSK_LREF_OR_VAL_T(args)... args) mutable -> task<pq_result_t<E, T>>
        {
            PGconn* h = self->checked_handle();

            DSK_TRY_SYNC init(h, DSK_MOVE_NON_LREF(args)...);

            // PQflush() will also try reading, but since we didn't wait for read ready,
            // it may miss it, though this works pretty well in practice.
            // https://github.com/postgres/postgres/blob/master/src/interfaces/libpq/fe-misc.c#L918-L926
            while(int r = PQflush(h))
            {
                if(r == -1)
                {
                    DSK_THROW(pq_errc::flush_failed);
                }

                DSK_TRY self->_skt.wait(decltype(self->_skt)::wait_type::wait_write);
            }

            if constexpr(E == pq_none) DSK_RETURN();
            else                       DSK_TRY_RETURN(self->do_get_result<E, T, StartCol, Row, true>());
        }(
            this, DSK_FORWARD(init), DSK_FORWARD(args)...
        );
    }

    template<pq_result_e E, class T, size_t StartCol, int Row, bool GetLast = false>
    auto do_get_result(auto&... vs) -> task<pq_result_t<E, std::conditional_t<(sizeof...(vs) > 0), bool, T>>>
    {
        static_assert(E != pq_none);
        static_assert((sizeof...(vs) && _void_<T>) || sizeof...(vs) == 0);

        pq_result r;

        for(PGconn* h = checked_handle();;)
        {
            while(PQisBusy(h))
            {
                DSK_TRY _skt.wait(decltype(_skt)::wait_type::wait_read);

                if(! PQconsumeInput(h))
                {
                    DSK_THROW(pq_errc::consume_input_failed);
                }
            }

            auto t = pq_result(PQgetResult(h));

            if(t)
            {
                if(auto ec = t.err())
                {
                    // Even when PQresultStatus indicates a fatal error, PQgetResult should be called until
                    // it returns a null pointer, to allow libpq to process the error information completely.
                    while(pq_result(PQgetResult(h)))
                    {}

                    DSK_THROW(ec);
                }

                if constexpr(E != pq_all)
                {
                    // For batched result, after the last row, or immediately if the query returns
                    // zero rows, a zero-row object with status PGRES_TUPLES_OK is returned
                    if(t.status() == PGRES_TUPLES_OK)
                    {
                        continue;
                    }
                }

                if constexpr(GetLast)
                {
                    // https://github.com/postgres/postgres/blob/master/src/interfaces/libpq/fe-exec.c#L2426

                    r = mut_move(t);

                    if(! is_oneof(r.status(), PGRES_COPY_IN, PGRES_COPY_OUT, PGRES_COPY_BOTH)
                        && conn_status() != CONNECTION_BAD)
                    {
                        continue;
                    }
                }
            }

            if constexpr(! GetLast)
            {
                r = mut_move(t);
            }

            break;
        }

        if constexpr(_void_<T> && sizeof...(vs) == 0)
        {
            if constexpr(E == pq_row) DSK_RETURN(pq_row_result(mut_move(r)));
            else                      DSK_RETURN_MOVED(r);
        }
        else
        {
            static_assert(E != pq_row || Row == 0 || Row == -1);

            if constexpr(sizeof...(vs))
            {
                if(! r)
                {
                    DSK_RETURN(false);
                }

                int nRow = r.row_cnt();
                int iRow = (Row >= 0 ? Row : nRow + Row);

                if(iRow < 0 || iRow >= nRow)
                {
                    DSK_THROW(errc::out_of_bound);
                }

                DSK_TRY_SYNC r.cols<StartCol>(iRow, vs...);
                DSK_RETURN(true);
            }
            else // ! _void_<T>
            {
                int nRow = r.row_cnt();
                int iRow = (Row >= 0 ? Row : nRow + Row);

                //if constexpr(requires{ requires _optional_<T> && _tuple_like_<typename T::value_type>; }) clang ICE
                if constexpr(DSK_CONDITIONAL_V(_optional_<T>, _tuple_like_<typename T::value_type>, false))
                {
                    if(iRow < 0 || iRow >= nRow)
                    {
                        DSK_RETURN(nullopt);
                    }

                    DSK_TRY_SYNC_RETURN(r.cols<typename T::value_type, StartCol>(iRow));
                }
                else
                {
                    if(iRow < 0 || iRow >= nRow)
                    {
                        DSK_THROW(errc::out_of_bound);
                    }

                    DSK_TRY_SYNC_RETURN(r.cols<T, StartCol>(iRow));
                }
            }
        }
    }

    template<bool AsStr>
    task<> do_send_copy(auto&&... bs)
    {
        static_assert(sizeof...(bs));

        return do_send<pq_none>([](PGconn* h, auto&&... bs)
        {
            return foreach_elms_until_err<pq_errc>(fwd_as_tuple(bs...), [&](auto& b)
            {
                int r = DSK_CONDITIONAL_V(AsStr, PQputCopyData(h, str_data<char>(b), str_size<int>(b)),
                                                 PQputCopyData(h, buf_data<char>(b), buf_size<int>(b)));

                     if(r ==  0) return pq_errc::send_copy_buf_full;
                else if(r == -1) return pq_errc::send_copy_failed;
                else             return pq_errc();
            });
        },
        DSK_FORWARD(bs)...);
    }

    static pq_errc set_batch(PGconn* h, int batch) noexcept
    {
        if(batch == 1)
        {
            if(! PQsetSingleRowMode(h))
            {
                return pq_errc::set_single_row_mode_failed;
            }
        }
        else if(batch > 1)
        {
            DSK_ASSERT(false); // TODO: waiting for vcpkg libpq update
            //if(! PQsetChunkedRowsMode(h, batch))
            //{
            //    return pq_errc::set_chunked_rows_mode_failed;
            //}
        }

        return {};
    }

    static constexpr auto do_add_exec = [](PGconn* h, int batch, _byte_str_ auto&& cmd, auto&&... params)
    {
        auto b = build_pq_params(params...);

        if constexpr(_pq_preapred_<decltype(cmd)>)
        {
            if(! PQsendQueryPrepared(h, cstr_data<char>(as_cstr(cmd)),
                                        b.count, b.vals, b.lens, b.fmts,
                                        pq_fmt_binary))
            {
                return pq_errc::send_query_prepared_failed;
            }
        }
        else
        {
            if(! PQsendQueryParams(h, cstr_data<char>(as_cstr(cmd)),
                                      b.count, b.oids, b.vals, b.lens, b.fmts,
                                      pq_fmt_binary))
            {
                return pq_errc::send_query_params_failed;
            }
        }

        return set_batch(h, batch);
    };

    static constexpr auto do_add_prepare_oids = [](PGconn* h, _byte_str_ auto const& name, _byte_str_ auto const& cmd,
                                                              std::convertible_to<Oid> auto const&... oids)
    {
        std::array<Oid, sizeof...(oids)> oidArr{static_cast<Oid>(oids)...};

        int r = PQsendPrepare(h, cstr_data<char>(as_cstr(name)),
                                 cstr_data<char>(as_cstr(cmd)),
                                 oidArr.size(), oidArr.data());

        if(! r) return pq_errc::send_prepare_failed;
        else    return pq_errc();
    };

    static constexpr auto do_add_describe_prepared = [](PGconn* h, _byte_str_ auto const& name)
    {
        int r = PQsendDescribePrepared(h, cstr_data<char>(as_cstr(name)));

        if(! r) return pq_errc::send_describe_prepared_failed;
        else    return pq_errc();
    };

    static constexpr auto do_add_describe_portal = [](PGconn* h, _byte_str_ auto const& name)
    {
        int r = PQsendDescribePortal(h, cstr_data<char>(as_cstr(name)));

        if(! r) return pq_errc::send_describe_portal_failed;
        else    return pq_errc();
    };

    static constexpr auto do_add_close_prepared = [](PGconn* h, _byte_str_ auto const& name)
    {
        (void)(h, name);
        DSK_ASSERT(false); // TODO: waiting for vcpkg libpq update
        return pq_errc::send_close_prepared_failed;

        //int r = PQsendClosePrepared(h, cstr_data<char>(as_cstr(name)));
        //
        //if(! r) return pq_errc::send_close_prepared_failed;
        //else    return pq_errc();
    };

    static constexpr auto do_add_close_portal = [](PGconn* h, _byte_str_ auto const& name)
    {
        (void)(h, name);
        DSK_ASSERT(false); // TODO: waiting for vcpkg libpq update
        return pq_errc::send_close_portal_failed;

        //int r = PQsendClosePortal(h, cstr_data<char>(as_cstr(name)));
        //
        //if(! r) return pq_errc::send_close_portal_failed;
        //else    return pq_errc();
    };

public:
    explicit pq_conn(auto&& exc) : _skt(DSK_FORWARD(exc)) {}

    pq_conn() : pq_conn(DSK_DEFAULT_IO_CONTEXT) {}

    ~pq_conn() { close(); }

    pq_conn(pq_conn&&) = default;

    error_code close()
    {
        DSK_E_TRY_ONLY(_skt.release());
        reset();
        return {};
    }

    char const* dbname  () const noexcept { return PQdb      (checked_handle()); }
    char const* username() const noexcept { return PQuser    (checked_handle()); }
    char const* password() const noexcept { return PQpass    (checked_handle()); }
    char const* host    () const noexcept { return PQhost    (checked_handle()); }
    char const* hostaddr() const noexcept { return PQhostaddr(checked_handle()); }
    char const* port    () const noexcept { return PQport    (checked_handle()); }
    char const* tty     () const noexcept { return PQtty     (checked_handle()); }
    char const* options () const noexcept { return PQoptions (checked_handle()); }

    int   protocol_ver  () const noexcept { return PQprotocolVersion        (checked_handle()); }
    int   server_ver    () const noexcept { return PQserverVersion          (checked_handle()); }
    int   backend_pid   () const noexcept { return PQbackendPID             (checked_handle()); }
    bool  needs_password() const noexcept { return PQconnectionNeedsPassword(checked_handle()); }
    bool  used_password () const noexcept { return PQconnectionUsedPassword (checked_handle()); }
    bool  used_gssapi   () const noexcept { return PQconnectionUsedGSSAPI   (checked_handle()); }
    bool  gss_enc_in_use() const noexcept { return PQgssEncInUse            (checked_handle()); }
    void* get_gss_ctx   () const noexcept { return PQgetgssctx              (checked_handle()); }
    bool  ssl_in_use    () const noexcept { return PQsslInUse               (checked_handle()); }
    void* get_ssl       () const noexcept { return PQgetssl                 (checked_handle()); }

    void* ssl_struct(_byte_str_ auto const& name) const
    {
        return PQsslStruct(checked_handle(), cstr_data<char>(as_cstr(name)));
    }

    char const* ssl_attr(_byte_str_ auto const& name) const
    {
        return PQsslAttribute(checked_handle(), cstr_data<char>(as_cstr(name)));
    }

    char const* const* ssl_attr_names() const noexcept
    {
        return PQsslAttributeNames(checked_handle());
    }

    int client_encoding() const noexcept
    {
        return PQclientEncoding(checked_handle());
    }

    char const* client_encoding_str() const noexcept
    {
        return pg_encoding_to_char(client_encoding());
    }

    bool set_client_encoding(_byte_str_ auto const& encoding)
    {
        return 0 == PQsetClientEncoding(checked_handle(), cstr_data<char>(as_cstr(encoding)));
    }

    bool set_client_encoding(int encoding)
    {
        return set_client_encoding(pg_encoding_to_char(encoding));
    }

    ConnStatusType conn_status() const noexcept
    {
        return PQstatus(checked_handle());
    }

    PGTransactionStatusType trxn_status() const noexcept
    {
        return PQtransactionStatus(checked_handle());
    }

    char const* param_status(_byte_str_ auto const& name) const
    {
        return PQparameterStatus(checked_handle(), cstr_data<char>(as_cstr(name)));
    }

    char const* err_msg() const noexcept
    {
        return PQerrorMessage(checked_handle());
    }

    PGVerbosity set_err_verbosity(PGVerbosity verbosity) noexcept
    {
        return PQsetErrorVerbosity(checked_handle(), verbosity);
    }

    PGContextVisibility set_err_ctx_visibility(PGContextVisibility visibility) noexcept
    {
        return PQsetErrorContextVisibility(checked_handle(), visibility);
    }

    void trace(FILE* debug_port) noexcept
    {
        PQtrace(checked_handle(), debug_port);
    }

    void untrace() noexcept
    {
        PQuntrace(checked_handle());
    }

    void set_trace_flags(int flags) noexcept
    {
        PQsetTraceFlags(checked_handle(), flags);
    }

    // Note: pass a null recv only return current receiver, won't set it to null.
    //       The default notice receiver just pass it to notice processor.
    PQnoticeReceiver notice_receiver(PQnoticeReceiver recv, void* arg = nullptr) noexcept
    {
        return PQsetNoticeReceiver(checked_handle(), recv, arg);
    }

    // NOTE: connect() will override notice processor as empty callback,
    //       because default notice processor prints message on stderr,
    //       which is not suitable for async task.
    //       So, if you want to set a notice processor, you should set it after connect()
    // NOTE: pass a null proc only return current processor, won't set it to null.
    PQnoticeProcessor notice_processor(PQnoticeProcessor proc, void* arg = nullptr) noexcept
    {
        return PQsetNoticeProcessor(checked_handle(), proc, arg);
    }


    bool escape_str(_byte_str_ auto const& s, _resizable_byte_buf_ auto& d)
    {
        size_t sn = str_size(s);
        rescale_buf(d, 2 * sn + 1);

        int err = 0;
        size_t n = PQescapeStringConn(checked_handle(), str_data<char>(d),
                                                        str_data<char>(s), sn, &err);
        resize_buf(d, n);
        return err == 0;
    }

    pq_str escape_literal(_byte_str_ auto const& s) noexcept
    {
        return PQescapeLiteral(checked_handle(), str_data<char>(s), str_size(s));
    }

    pq_str escape_identifier(_byte_str_ auto const& s) noexcept
    {
        return pq_str(PQescapeIdentifier(checked_handle(), str_data<char>(s), str_size(s)));
    }

    pq_str escape_bytea(_byte_buf_ auto const& b) noexcept
    {
        size_t n = 0;
        auto*  d = PQescapeByteaConn(checked_handle(), buf_data<unsigned char>(b), buf_size(b), &n);
        return pq_str(static_cast<char*>(d));
    }

    static pq_buf unescape_bytea_str(_byte_str_ auto const& s) noexcept
    {
        size_t n = 0;
        auto*  d = PQunescapeBytea(cstr_data<unsigned char>(as_cstr(s)), &n);
        return pq_buf(d, n);
    }

    pq_str encrypt_password(_byte_str_ auto const& passwd, _byte_str_ auto const& user, _byte_str_ auto const&... algo)
    {
        static_assert(sizeof...(algo) <= 1);

        return pq_str(PQencryptPasswordConn(checked_handle(),
            cstr_data<char>(as_cstr(passwd)),
            cstr_data<char>(as_cstr(user)),
            (nullptr, ..., cstr_data<char>(as_cstr(algo)))));
    }


    // https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
    // https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PQCONNECTSTARTPARAMS
    task<> connect(_byte_str_ auto&&... args)
    {
        return do_connect<PQconnectPoll>([this](auto&&... args) -> error_code
        {
            DSK_E_TRY_ONLY(close());

            if constexpr(sizeof...(args) == 1)
            {
                reset_handle(PQconnectStart(cstr_data<char>(as_cstr(args))...));
            }
            else
            {
                static_assert(sizeof...(args) && sizeof...(args) % 2 == 0);

                char const* end = nullptr;
                auto[ks, vs] = partition_args(DSK_TYPE_IDX_EXP(I%2 == 0), args..., end, end);

                reset_handle(PQconnectStartParams(
                    transform_elms(ks, DSK_ELM_EXP(cstr_data<char>(as_cstr(e)))).template as_array<char*>(), 
                    transform_elms(vs, DSK_ELM_EXP(cstr_data<char>(as_cstr(e)))).template as_array<char*>(),
                    0));
            }

            if(PQstatus(handle()) == CONNECTION_BAD)
            {
                return pq_errc::connect_init_failed;
            }

            if(PQsetnonblocking(handle(), 1))
            {
                return pq_errc::set_non_blocking_failed;
            }
        
            PQsetNoticeProcessor(handle(), +[](void*, char const*) {}, nullptr);

            return {};
        },
        DSK_FORWARD(args)...);
    }

    task<> reconnect()
    {
        return do_connect<PQresetPoll>([this]() -> error_code
        {
            DSK_E_TRY_ONLY(_skt.release());

            if(! PQresetStart(checked_handle())) return pq_errc::connect_reset_failed;
            else                                 return {};
        });
    }

    // can include multiple SQL commands (separated by semicolons), they are
    // processed in a single transaction, unless there are explicit BEGIN/COMMIT
    // commands included in the query string to divide it into multiple transactions.
    // If one of the commands fail, processing of the string stops with it and
    // the returned PGresult describes the error condition.
    // If batch > 0, returns at most batch rows in each result, otherwise all in one result.
    // NOTE: only returns text format result
    template<pq_result_e E = pq_none, class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<E, T>> start_exec_multi(_byte_str_ auto&& cmds, int batch = 0)
    {
        DSK_ASSERT(E == pq_none || E == batch || (E > 1 && batch > 1));

        return do_send<E, T, StartCol, Row>([](PGconn* h, int batch, auto&& cmds)
        {
            if(! PQsendQuery(h, cstr_data<char>(as_cstr(cmds))))
            {
                return pq_errc::send_query_failed;
            }

            return set_batch(h, batch);
        },
        std::move(batch), DSK_FORWARD(cmds));
    }

    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_all, T>> exec_multi(_byte_str_ auto&& cmds, int batch = 0)
    {
        return start_exec_multi<pq_all, T, StartCol, Row>(DSK_FORWARD(cmds), batch);
    }

    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_batch, T>> exec_multi_return_last_batch(_byte_str_ auto&& cmds, int batch)
    {
        return start_exec_multi<pq_batch, T, StartCol, Row>(DSK_FORWARD(cmds), batch);
    }

    template<class T = void, size_t StartCol = 0>
    task<pq_result_t<pq_row, T>> exec_multi_return_last_row(_byte_str_ auto&& cmds)
    {
        return start_exec_multi<pq_row, T, StartCol>(DSK_FORWARD(cmds), 1);
    }


    // Most parameter types are in binary format.
    // Use pq_tparam to force text parameter.
    // Always returns binary format result.
    template<pq_result_e E = pq_none, class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<E, T>> start_exec(int batch, _byte_str_ auto&& cmd, auto&&... params)
    {
        DSK_ASSERT(E == pq_none || E == batch || (E > 1 && batch > 1));

        return do_send<E, T, StartCol, Row>(do_add_exec,
                                            std::move(batch),
                                            DSK_FORWARD(cmd),
                                            DSK_FORWARD(params)...);
    }

    task<void> start_exec(_byte_str_ auto&& cmd, auto&&... params)
    {
        return start_exec(0, DSK_FORWARD(cmd), DSK_FORWARD(params)...);
    }

    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_all, T>> exec(_byte_str_ auto&& cmd, auto&&... params)
    {
        return start_exec<pq_all, T, StartCol, Row>(0, DSK_FORWARD(cmd), DSK_FORWARD(params)...);
    }

    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_batch, T>> exec_return_last_batch(int batch, _byte_str_ auto&& cmd, auto&&... params)
    {
        return start_exec<pq_batch, T, StartCol, Row>(batch, DSK_FORWARD(cmd), DSK_FORWARD(params)...);
    }

    template<class T = void, size_t StartCol = 0>
    task<pq_result_t<pq_row, T>> exec_return_last_row(_byte_str_ auto&& cmd, auto&&... params)
    {
        return start_exec<pq_row, T, StartCol>(1, DSK_FORWARD(cmd), DSK_FORWARD(params)...);
    }

    /// fetch results
    ///
    // when no cmd undergoing, xxx_result() just returns nullptr result
    
    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_all, T>> next_result()
    {
        return do_get_result<pq_all, T, StartCol, Row>();
    }

    template<size_t StartCol = 0, int Row = 0>
    task<bool> next_result(auto& v, auto&... vs)
    {
        return do_get_result<pq_all, void, StartCol, Row>(v, vs...);
    }

    // can be used if inited with batch > 1
    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_batch, T>> next_batch()
    {
        return do_get_result<pq_batch, T, StartCol, Row>();
    }

    template<size_t StartCol = 0, int Row = 0>
    task<bool> next_batch(auto& v, auto&... vs)
    {
        return do_get_result<pq_batch, void, StartCol, Row>(v, vs...);
    }

    // can be used if inited with batch == 1
    template<class T = void, size_t StartCol = 0>
    task<pq_result_t<pq_row, T>> next_row()
    {
        return do_get_result<pq_row, T, StartCol, 0>();
    }

    template<size_t StartCol = 0>
    task<bool> next_row(auto& v, auto&... vs)
    {
        return do_get_result<pq_row, void, StartCol, 0>(v, vs...);
    }


    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_all, T>> last_result()
    {
        return do_get_result<pq_all, T, StartCol, Row, true>();
    }

    template<size_t StartCol = 0, int Row = 0>
    task<bool> last_result(auto& v, auto&... vs)
    {
        return do_get_result<pq_all, void, StartCol, Row, true>(v, vs...);
    }

    // can be used if inited with batch > 1
    template<class T = void, size_t StartCol = 0, int Row = 0>
    task<pq_result_t<pq_batch, T>> last_batch()
    {
        return do_get_result<pq_batch, T, StartCol, Row, true>();
    }

    template<size_t StartCol = 0, int Row = 0>
    task<bool> last_batch(auto& v, auto&... vs)
    {
        return do_get_result<pq_batch, void, StartCol, Row, true>(v, vs...);
    }

    // can be used if inited with batch == 1
    template<class T = void, size_t StartCol = 0>
    task<pq_result_t<pq_row, T>> last_row()
    {
        return do_get_result<pq_row, T, StartCol, 0, true>();
    }

    template<size_t StartCol = 0>
    task<bool> last_row(auto& v, auto&... vs)
    {
        return do_get_result<pq_row, void, StartCol, 0, true>(v, vs...);
    }


    // name must be unique for the session;
    // only unnamed statement (name = "") will automatically replace
    // pre-existing unnamed statement;
    // use exec(pq_prepared("stmt name"), ...) to execute prepared statement.
    task<pq_result> prepare_oids(_byte_str_ auto&& name, _byte_str_ auto&& cmd,
                                 std::convertible_to<Oid> auto&&... oids)
    {
        return do_send<pq_all>(do_add_prepare_oids, DSK_FORWARD(name), DSK_FORWARD(cmd), DSK_FORWARD(oids)...);
    }

    template<class... Params>
    task<pq_result> prepare_for(_byte_str_ auto&& name, _byte_str_ auto&& cmd)
    {
        return prepare_oids(DSK_FORWARD(name), DSK_FORWARD(cmd), pq_param_oid<Params>...);
    }

    task<pq_result> prepare(_byte_str_ auto&& name, _byte_str_ auto&& cmd, auto&&... params)
    {
        return prepare_for<DSK_NO_CVREF_T(params)...>(DSK_FORWARD(name), DSK_FORWARD(cmd));
    }

    // On success, a result with status PGRES_COMMAND_OK is returned.
    // The result methods param_cnt() and param_oid() can be used to obtain
    // information about the parameters of the prepared statement, and the
    // methods fld_cnt(), fld_name(), fld_oid(), etc. provide
    // information about the result columns (if any) of the statement.
    task<pq_result> describe_prepared(_byte_str_ auto&& name)
    {
        return do_send<pq_all>(do_add_describe_prepared, DSK_FORWARD(name));
    }

    // On success, a result with status PGRES_COMMAND_OK is returned.
    // The result methods fld_cnt(), fld_name(), fld_oid(), etc. provide
    // information about the result columns (if any) of the portal.
    task<pq_result> describe_portal(_byte_str_ auto&& name)
    {
        return do_send<pq_all>(do_add_describe_portal, DSK_FORWARD(name));
    }


    task<pq_result> close_prepared(_byte_str_ auto&& name)
    {
        return do_send<pq_all>(do_add_close_prepared, DSK_FORWARD(name));
    }

    task<pq_result> close_portal(_byte_str_ auto&& name)
    {
        return do_send<pq_all>(do_add_close_portal, DSK_FORWARD(name));
    }


    template<class Alloc>
    class transaction_t
    {
        struct data_t
        {
            pq_conn& conn;
            bool done = false;
        };

        allocated_unique_ptr<data_t, Alloc> _d;

    public:
        transaction_t(transaction_t&&) = default;

        explicit transaction_t(pq_conn& conn)
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
    task<transaction_t<Alloc>> make_transaction()
    {
        DSK_TRY exec("BEGIN");

        transaction_t<Alloc> trxn(*this);

        DSK_WAIT add_parent_cleanup(trxn.rollback_if_uncommited());

        DSK_RETURN_MOVED(trxn);
    }

    /// COPY
    ///
    // https://www.postgresql.org/docs/current/sql-copy.html

    task<pq_result> start_copy_to(_byte_str_ auto const& target, _byte_str_ auto const&... opts_conds)
    {
        static_assert(sizeof...(opts_conds) <= 1);

        return exec(cat_str("COPY ", target, " FROM STDIN ", opts_conds...));
    }

    task<pq_result> start_copy_from(_byte_str_ auto const& src, _byte_str_ auto const&... opts)
    {
        static_assert(sizeof...(opts) <= 1);

        return exec(cat_str("COPY ", src, " TO STDOUT ", opts...));
    }

    task<> send_copy_buf(_byte_buf_ auto&&... bs)
    {
        return do_send_copy<false>(DSK_FORWARD(bs)...);
    }

    task<> send_copy_str(_byte_str_ auto&&... strs)
    {
        return do_send_copy<true>(DSK_FORWARD(strs)...);
    }

    task<pq_result> send_copy_end(_byte_str_ auto&&... err)
    {
        return do_send<pq_all>([](PGconn* h, auto&&... err)
        {
            static_assert(sizeof...(err) <= 1);

            int r = PQputCopyEnd(h, (nullptr, ..., cstr_data<char>(as_cstr(err))));

                 if(r ==  0) return pq_errc::send_copy_end_buf_full;
            else if(r == -1) return pq_errc::send_copy_end_failed;
            else             return pq_errc();
        },
        as_cstr(DSK_FORWARD(err))...);
    }

    task<pq_buf> get_copy_row()
    {
        PGconn* h = checked_handle();

        char* buf = nullptr;
        int r = 0;

        for(;;)
        {
            r = PQgetCopyData(h, &buf, 1);

            if(r > 0)
            {
                break;
            }
            else if(r == 0)
            {
                DSK_TRY _skt.wait(decltype(_skt)::wait_type::wait_read);

                if(! PQconsumeInput(h))
                {
                    DSK_THROW(pq_errc::consume_input_failed);
                }
            }
            else if(r == -1)
            {
                DSK_TRY last_result();
                buf = nullptr;
                r = 0;
                break;
            }
            else if(r == -2)
            {
                DSK_THROW(pq_errc::get_copy_row_failed);
            }
            else
            {
                DSK_ASSERT(false);
            }
        }

        DSK_RETURN(pq_buf(buf, r));
    }

    /// notify
    /// 
    // NOTE: A transaction that has executed NOTIFY cannot be prepared for two-phase commit.

    task<pq_result> listen(_byte_str_ auto&& channel)
    {
        return exec(cat_str("LISTEN ", channel));
    }

    task<pq_result> unlisten(_byte_str_ auto&& channel)
    {
        return exec(cat_str("UNLISTEN ", channel));
    }

    task<pq_result> notify(_byte_str_ auto&& channel, auto&&... payload)
    {
        static_assert(sizeof...(payload) <= 1);

        return exec((sizeof...(payload) ? "SELECT pg_notify($1, $2::TEXT)" : "SELECT pg_notify($1, '')"),
                    DSK_FORWARD(channel), DSK_FORWARD(payload)...);
    }

    task<pq_notify> get_notify()
    {
        PGconn* h = checked_handle();

        pq_notify r;

        while(! (r = pq_notify(PQnotifies(h))))
        {
            DSK_TRY _skt.wait(decltype(_skt)::wait_type::wait_read);

            if(! PQconsumeInput(h))
            {
                DSK_THROW(pq_errc::consume_input_failed);
            }
        }

        DSK_RETURN_MOVED(r);
    }

    // Any method that calls PQconsumeInput() will also try receiving notifies.
    // So, you can interleave this method with others.
    pq_notify try_get_notify() noexcept
    {
        return pq_notify(PQnotifies(checked_handle()));
    }


    /// pipeline
    /// 
    // NOTE: Succeeded cmds in pipeline will each return some results until a nullptr result.
    //       Each pipeline_sync() also add a commit cmd, though the result for
    //       it is combined with sync as PGRES_PIPELINE_SYNC, followed by a nullptr result.
    // NOTE: If error occurs, previous uncommitted/unsynced cmds in pipeline will be discarded,
    //       The failed cmd will THROW(for DSK_TRY) or return(for DSK_WAIT) the error, WITHOUT nullptr result being followed.
    //       Following discarded cmds will each return a PGRES_PIPELINE_ABORTED result followed by a nullptr result.
    //       Drain all these results until PGRES_PIPELINE_SYNC followed by a nullptr result.
    //       Then, the pipeline will be ready to execute new cmds.
    //       Though, in most cases, user will simply ditch the connection without draining.

    PGpipelineStatus pipeline_status() const noexcept { return PQpipelineStatus(checked_handle()); }

    pq_errc enter_pipeline_mode() noexcept
    {
        if(! PQenterPipelineMode(checked_handle())) return pq_errc::enter_pipeline_mode_failed;
        else                                        return pq_errc();
    }

    pq_errc exit_pipeline_mode() noexcept
    {
        if(! PQexitPipelineMode(checked_handle())) return pq_errc::exit_pipeline_mode_failed;
        else                                       return pq_errc();
    }

    task<> flush_request() noexcept
    {
        return do_send<pq_none>([](PGconn* h)
        {
            if(! PQsendFlushRequest(h)) return pq_errc::send_flush_request_failed;
            else                        return pq_errc();
        });
    }

    task<> pipeline_sync() noexcept
    {
        return do_send<pq_none>([](PGconn* h)
        {
            // TODO: waiting for vcpkg libpq update
            //if(! PQsendPipelineSync(h)) return pq_errc::send_pipeline_sync_failed;
            if(! PQpipelineSync(h)) return pq_errc::send_pipeline_sync_failed;
            else                    return pq_errc();
        });
    }

    pq_errc add_exec(int batch, _byte_str_ auto const& cmd, auto const&... params)
    {
        return do_add_exec(checked_handle(), batch, cmd, params...);
    }

    pq_errc add_exec(_byte_str_ auto const& cmd, auto const&... params)
    {
        return add_exec(0, cmd, params...);
    }

    pq_errc add_prepare_oids(_byte_str_ auto const& name, _byte_str_ auto const& cmd,
                             std::convertible_to<Oid> auto const&... oids)
    {
        return do_add_prepare_oids(checked_handle(), name, cmd, oids...);
    }

    template<class... Params>
    pq_errc add_prepare_for(_byte_str_ auto const& name, _byte_str_ auto const& cmd)
    {
        return add_prepare_oids(name, cmd, pq_param_oid<Params>...);
    }

    pq_errc add_prepare(_byte_str_ auto const& name, _byte_str_ auto const& cmd, auto const&... params)
    {
        return add_prepare_for<DSK_NO_CVREF_T(params)...>(name, cmd);
    }

    pq_errc add_describe_prepared(_byte_str_ auto const& name)
    {
        return do_add_describe_prepared(name);
    }

    pq_errc add_describe_portal(_byte_str_ auto const& name)
    {
        return do_add_describe_portal(name);
    }

    pq_errc add_close_prepared(_byte_str_ auto const& name)
    {
        return do_add_close_prepared(name);
    }

    pq_errc add_close_portal(_byte_str_ auto const& name)
    {
        return do_add_close_portal(name);
    }
};


} // namespace dsk
