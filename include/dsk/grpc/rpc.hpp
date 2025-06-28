#pragma once

#include <dsk/task.hpp>
#include <dsk/async_op.hpp>
#include <dsk/util/ct.hpp>
#include <dsk/util/str.hpp>
#include <dsk/grpc/cq.hpp>
#include <dsk/grpc/err.hpp>
#include <dsk/grpc/srv.hpp>

#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/generic/async_generic_service.h>


namespace dsk{


template<_byte_ T = char>
auto str_view(grpc::Slice& sl) noexcept
{
    return std::basic_string_view<T>(as_buf_of<T>(sl));
}

inline expected<grpc::Slice, grpc::StatusCode> uncompress_and_merge(grpc::ByteBuffer& b)
{
    grpc::Slice sl;

    if(has_err(b.TrySingleSlice(&sl)))
    {
        DSK_E_TRY_ONLY(b.DumpToSingleSlice(&sl));
    }

    return sl;
}

inline grpc::ByteBuffer to_grpc_buf_view_coll(_byte_buf_ auto&&... bs)
{
    constexpr size_t n = sizeof...(bs);

    grpc::Slice sl[n] = { grpc::Slice(buf_data(bs), buf_byte_size(bs), grpc::Slice::STATIC_SLICE)... };

    return grpc::ByteBuffer(&sl, n);
}

inline grpc::ByteBuffer to_grpc_buf_coll(_byte_buf_ auto&&... bs)
{
    constexpr size_t n = sizeof...(bs);

    grpc::Slice sl[n] = { grpc::Slice(buf_data(bs), buf_byte_size(bs))... };

    return grpc::ByteBuffer(sl, n);
}


template<auto Method>
using grpc_ctx_t = typename decltype
(
    []<class R, class S, class Ctx, class... Args>
    (R (S::*)(Ctx*, Args...))
    {
        return type_c<Ctx>;
    }(Method)
)::type;


// stub or service
template<auto Method>
using grpc_provider_t = typename decltype
(
    []<class R, class S, class Ctx, class... Args>
    (R (S::*)(Ctx*, Args...))
    {
        return type_c<S>;
    }(Method)
)::type;


template<auto Method>
using grpc_delegator_t = typename decltype
(
    []<class R, class S, class Ctx, class... Args>
    (R (S::*)(Ctx*, Args...))
    {
        if constexpr(_derived_from_<Ctx, grpc::ClientContext>)
        {
            return type_c<typename R::element_type>;
        }
        else
        {
            static_assert(_derived_from_<Ctx, grpc::ServerContext>);

            return std::remove_pointer<
                type_pack_elm<
                    ct_find_first(type_list<Args...>, DSK_TYPE_EXP(
                                  std::is_convertible_v<T, grpc::internal::ServerAsyncStreamingInterface*>))[0],
                    Args...>>();
        }
    }(Method)
)::type;


template<size_t I, auto Method>
using grpc_nth_ncvp_param = typename decltype
(
    []<class R, class S, class... Args>
    (R (S::*)(Args...))
    {
        return std::remove_cvref<std::remove_pointer_t<type_pack_elm<I, Args...>>>();
    }(Method)
)::type;



// Method: AsyncXXX, not PrepareAsyncXXX, except for generic ones.
template<auto Method, class Cq = grpc_cq>
class grpc_rpc
{
public:
    using provider_type  = grpc_provider_t<Method>;
    using ctx_type       = grpc_ctx_t<Method>;
    using delegator_type = grpc_delegator_t<Method>;

    static constexpr bool isClient = _derived_from_<ctx_type, grpc::ClientContext>;
    static constexpr bool isServer = ! isClient;

    static constexpr bool isClientUnary        = _specialized_from_<delegator_type, grpc::ClientAsyncResponseReader>;
    static constexpr bool isClientClientStream = _specialized_from_<delegator_type, grpc::ClientAsyncWriter>;
    static constexpr bool isClientServerStream = _specialized_from_<delegator_type, grpc::ClientAsyncReader>;
    static constexpr bool isClientBidiStream   = _specialized_from_<delegator_type, grpc::ClientAsyncReaderWriter>;

    static constexpr bool isServerUnary        = _specialized_from_<delegator_type, grpc::ServerAsyncResponseWriter>;
    static constexpr bool isServerClientStream = _specialized_from_<delegator_type, grpc::ServerAsyncReader>;
    static constexpr bool isServerServerStream = _specialized_from_<delegator_type, grpc::ServerAsyncWriter>;
    static constexpr bool isServerBidiStream   = _specialized_from_<delegator_type, grpc::ServerAsyncReaderWriter>;

    static constexpr bool isGenericClient = _specialized_from_<provider_type, grpc::TemplatedGenericStub>;

    using request_type = typename decltype([]()
    {
        if constexpr(isClientUnary || isClientServerStream
                  || isServerUnary || isServerServerStream)
        {
            return type_c<grpc_nth_ncvp_param<isGenericClient ? 2 : 1, Method>>;
        }
        else
        {
            return type_c<type_list_elm<isClient ? 0 : 1, delegator_type>>;
        }
    }())::type;

    using response_type = typename decltype([]()
    {
        if constexpr(isClientClientStream)
        {
            return type_c<grpc_nth_ncvp_param<1, Method>>;
        }
        else
        {
            return type_c<type_list_elm<isClientBidiStream ? 1 : 0, delegator_type>>;
        }
    }())::type;

private:
    struct data_t
    {
        provider_type& prd;
        Cq&            cq;
        ctx_type       ctx;
        DSK_CONDITIONAL_T(isClient, std::unique_ptr<delegator_type>, delegator_type) dgt; // must defined after ctx
        DSK_DEF_MEMBER_IF(isClient, grpc::Status) sts;
        DSK_DEF_MEMBER_IF(isGenericClient, std::string) name;

        data_t(provider_type& p, Cq& q, auto&&... n) requires isClient
            : prd(p), cq(q), name(DSK_FORWARD(n)...)
        {}

        data_t(provider_type& p, Cq& q, auto&&... n) requires isServer
            : prd(p), cq(q), dgt(&ctx), name(DSK_FORWARD(n)...)
        {}

        auto& dref() noexcept
        {
            if constexpr(requires{ *dgt; }) return *dgt;
            else                            return  dgt;
        }
    };

    data_t _d;

    enum client_finish_mode_e
    {
        client_finish_non_ok,
        client_finish_skip,
        client_finish_forced
    };

    template<class Init, class R, bool ServerCancellable>
    struct init_op
    {
        static constexpr bool isCancellable = isClient || ServerCancellable;

                                          data_t& _d;
        DSK_NO_UNIQUE_ADDR                  Init  _init;
        DSK_DEF_MEMBER_IF(! _void_<R>,         R) _r{};
        DSK_DEF_MEMBER_IF(   isServer, grpc_errc) _err{};
        continuation                             _cont;
        DSK_DEF_MEMBER_IF(isCancellable, optional_stop_callback) _scb; // must be last one defined

        auto err_code() const noexcept
        {
            if constexpr(isClient) return _d.sts.error_code();
            else                   return _err;
        }

        using is_async_op = void;

        bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
        {
            if constexpr(isClient) DSK_ASSERT(_d.sts.ok());
            else                   DSK_ASSERT(! has_err(_err));

            if(stop_requested(ctx))
            {
                if constexpr(isClient) _d.sts = grpc::Status::CANCELLED;
                else                   _err = grpc_errc::server_call_canceled;
                return false;
            }

            _cont = DSK_FORWARD(cont);

            if constexpr(isCancellable)
            {
                if(stop_possible(ctx))
                {
                    _scb.emplace(get_stop_token(ctx), [this]()
                    {
                        // NOTE: You can't cancel server rpc start.
                        //       You have to shut down the server.
                        //       Canceling a client start will still
                        //       wait until server replied or timeout.
                        _d.ctx.TryCancel();
                    });
                }
            }

            _init(*this, ctx);
            return true;
        }

        bool is_failed() const noexcept
        {
            return has_err(err_code());
        }

        auto take_result() noexcept
        {
            if constexpr(_void_<R>) return make_expected_if_no(err_code());
            else                    return make_expected_if_no(err_code(), mut_move(_r));
        }

        decltype(auto) to_cq_cb(auto&& f)
        {
            if constexpr(requires{ f((void*)0); })
            {
                return DSK_FORWARD(f);
            }
            else if constexpr(requires{ f(_d, (void*)0); })
            {
                return [this, f = DSK_FORWARD(f)](void* tag) mutable
                {
                    f(_d, tag);
                };
            }
            else if constexpr(requires{ f(_d, _r, (void*)0); })
            {
                return [this, f = DSK_FORWARD(f)](void* tag) mutable
                {
                    f(_d, _r, tag);
                };
            }
            else
            {
                static_assert(false, "unsupported type");
            }
        }

        void init_client_finish(auto&& ctx, auto&&... args) requires isClient
        {
            _d.cq.post
            (
                [&](void* tag)
                {
                    _d.dgt->Finish(args..., &_d.sts, tag);
                },
                [this, ctx](bool ok) mutable
                {
                    DSK_ASSERT(ok);
                    resume(mut_move(_cont), ctx);
                }
            );
        }

        template<client_finish_mode_e Fm>
        void init_client_act(auto&& ctx, auto&& init) requires isClient
        {
            _d.cq.post(to_cq_cb(DSK_FORWARD(init)), [this, ctx](bool ok) mutable
            {
                     if constexpr(_same_as_<R, bool>) { _r = ok; }
                else if constexpr(_optional_<R>     ) { if(! ok) _r.reset(); }

                if constexpr(Fm == client_finish_non_ok)
                {
                    if(ok) resume(mut_move(_cont), ctx);
                    else   init_client_finish(ctx);
                }
                else if constexpr(Fm == client_finish_skip)
                {
                    resume(mut_move(_cont), ctx);
                }
                else
                {
                    static_assert(Fm == client_finish_forced);
                    init_client_finish(ctx);
                }
            });
        }

        template<grpc_errc ErrOnNonOk>
        void init_server_act(auto&& ctx, auto&& init) requires isServer
        {
            _d.cq.post(to_cq_cb(DSK_FORWARD(init)), [this, ctx](bool ok) mutable
            {
                if constexpr(_same_as_<R, bool>) _r = ok;

                if constexpr(has_err(ErrOnNonOk) || _optional_<R>)
                {
                    if(! ok)
                    {
                        if constexpr(has_err(ErrOnNonOk)) _err = ErrOnNonOk;
                        else                              DSK_ASSERT(false);

                        if constexpr(_optional_<R>) _r.reset();
                    }
                }

                resume(mut_move(_cont), ctx);
            });
        }

        void init_get_response(_async_ctx_ auto&& ctx,
                               request_type const& req, response_type& res) requires isClientUnary
        {
            if constexpr(isGenericClient)
            {
                _d.dgt = (_d.prd.*Method)(&_d.ctx, _d.name, req, _d.cq.cq());
                _d.dgt->StartCall();
            }
            else
            {
                _d.dgt = (_d.prd.*Method)(&_d.ctx, req, _d.cq.cq());
            }

            init_client_finish(ctx, &res);
        }

        void init_get_request(_async_ctx_ auto&& ctx, request_type& req) requires isServerUnary || isServerServerStream
        {
            init_server_act<grpc_errc::server_call_start_failed>(ctx, [&](void* tag)
            {
                (_d.prd.*Method)(&_d.ctx, &req, &_d.dgt, _d.cq.cq(), _d.cq.srv_cq(), tag);
            });
        }
    };

    template<class R = void, bool ServerCancellable = true>
    auto make_init_op(auto&& init)
    {
        return init_op<DSK_DECAY_T(init), R, ServerCancellable>(_d, DSK_FORWARD(init));
    }

    template<
        client_finish_mode_e Fm = client_finish_non_ok,
        class R = void
    >
    auto make_client_op(auto&& init)
    {
        return make_init_op<R>([init = DSK_FORWARD(init)](auto& op, auto&& ctx) mutable
        {
            op.template init_client_act<Fm>(DSK_FORWARD(ctx), mut_move(init));
        });
    }

    template<
        grpc_errc ErrOnNonOk = {},
        class R = void,
        bool Cancellable = true
    >
    auto make_server_op(auto&& init)
    {
        return make_init_op<R, Cancellable>([init = DSK_FORWARD(init)](auto& op, auto&& ctx) mutable
        {
            op.template init_server_act<ErrOnNonOk>(ctx, mut_move(init));
        });
    }

    template<
        grpc_errc ServerErrOnNonOk = {},
        class R = void,
        client_finish_mode_e ClientFinishMode = client_finish_non_ok,
        bool ServerCancellable = true
    >
    auto make_act_op(auto&& init)
    {
        return make_init_op<R, ServerCancellable>([init = DSK_FORWARD(init)](auto& op, auto&& ctx) mutable
        {
            if constexpr(isClient) op.template init_client_act<ClientFinishMode>(ctx, mut_move(init));
            else                   op.template init_server_act<ServerErrOnNonOk>(ctx, mut_move(init));
        });
    }

    auto start_op(auto&&... args) requires(! isClientUnary)
    {
        return make_act_op
        <
            grpc_errc::server_call_start_failed,
            void,
            client_finish_non_ok,
            false // server start cannot be cancelled
        >
        (
            [this, args = fwd_lref_decay_other_as_tuple(DSK_FORWARD(args)...)](void* tag) mutable
            {
                apply_elms(mut_move(args), [&](auto&&... args)
                {
                    if constexpr(isClient)
                    {
                        if constexpr(isGenericClient)
                        {
                            _d.dgt = (_d.prd.*Method)(&_d.ctx, _d.name, DSK_FORWARD(args)..., _d.cq.cq());
                            _d.dgt->StartCall(tag);
                        }
                        else
                        {
                            _d.dgt = (_d.prd.*Method)(&_d.ctx, DSK_FORWARD(args)..., _d.cq.cq(), tag);
                        }
                    }
                    else
                    {
                        (_d.prd.*Method)(&_d.ctx, DSK_FORWARD(args)..., &_d.dgt, _d.cq.cq(), _d.cq.srv_cq(), tag);
                    }
                });
            }
        );
    }

    auto finish_op(auto&&... args) requires(! isClientUnary)
    {
        return make_init_op
        (
            [args = fwd_lref_decay_other_as_tuple(DSK_FORWARD(args)...)](auto& op, auto&& ctx) mutable
            {
                if constexpr(isClient)
                {
                    DSK_ASSERT(! args.count);
                    op.init_client_finish(ctx);
                }
                else
                {
                    op.template init_server_act<grpc_errc::server_call_finish_failed>(ctx, [&](void* tag)
                    {
                        apply_elms(mut_move(args), [&](auto&&... args)
                        {
                            op._d.dgt.Finish(DSK_FORWARD(args)..., tag);
                        });
                    });
                }
            }
        );
    }

public:
    grpc_rpc(provider_type& serviceOrStub, Cq& cq) requires(! isGenericClient)
        :  _d(serviceOrStub, cq)
    {}

    grpc_rpc(_char_str_ auto&& name, provider_type& stub, Cq& cq) requires isGenericClient
        :  _d(stub, cq, DSK_FORWARD(name))
    {
        DSK_ASSERT(! str_empty(name));
    }

    template<class Services>
    explicit grpc_rpc(grpc_srv_t<Services, Cq>& srv) requires isServer
        : grpc_rpc(srv.template service<provider_type>(), srv.cq())
    {}

    auto& status() const noexcept requires isClient { return _d.sts; }
    auto& context(this auto& self) noexcept { return self._d.ctx; }

    // only valid after successful recv_initial_metadata()
    auto& initial_metadata() const noexcept requires isClient
    {
        return _d.ctx.GetServerInitialMetadata();
    }

    // only valid after successful finish (or get_response for unary rpc)
    auto& trailing_metadata() const noexcept requires isClient
    {
        return _d.ctx.GetServerTrailingMetadata();
    }

    void add_initial_metadata(_char_str_ auto&& k, _char_str_ auto&& v) requires isServer
    {
        _d.ctx.AddInitialMetadata(DSK_FORWARD(k), DSK_FORWARD(v));
    }

    void add_trailing_metadata(_char_str_ auto&& k, _char_str_ auto&& v) requires isServer
    {
        _d.ctx.AddTrailingMetadata(DSK_FORWARD(k), DSK_FORWARD(v));
    }

// AsyncNotifyWhenDone is ill-formed, as the tag will
// only be returned if rpc starts and the order with
// other tags of same rpc is undetermined.
//     // Must be called before rpc starts.
//     // On resume, context.IsCancelled() can then be
//     // called to check whether rpc was cancelled.
//     auto wait_until_done() requires isServer
//     {
//         return make_act_op([this](void* tag)
//         {
//             _d.dgt->AsyncNotifyWhenDone(tag);
//         });
//     }

    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto get_response(auto&& req, response_type& res) requires isClientUnary
    {
        return make_init_op
        (
            [req = wrap_lref_fwd_other(DSK_FORWARD(req)), &res](auto& op, auto&& ctx) mutable
            {
                op.init_get_response(ctx, unwrap_ref_or_move(req), res);
            }
        );
    }

    // result type: expected<response_type, grpc::StatusCode>
    [[nodiscard]] auto get_response(auto&& req) requires isClientUnary
    {
        return make_init_op<response_type>
        (
            [req = wrap_lref_fwd_other(DSK_FORWARD(req))](auto& op, auto&& ctx) mutable
            {
                op.init_get_response(ctx, unwrap_ref_or_move(req), op._r);
            }
        );
    }


    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto get_request(request_type& req) requires isServerUnary || isServerServerStream
    {
        return make_init_op<void, false>([&req](auto& op, auto&& ctx)
        {
            op.init_get_request(ctx, req);
        });
    }

    // result type: expected<request_type, grpc_errc>
    [[nodiscard]] auto get_request() requires isServerUnary || isServerServerStream
    {
        return make_init_op<request_type, false>([](auto& op, auto&& ctx)
        {
            op.init_get_request(ctx, op._r);
        });
    }


    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto send_request(auto&& req) requires isClientServerStream
    {
        return start_op(DSK_FORWARD(req));
    }


    // result type: expected<void, grpc::StatusCode for client / grpc_errc for server>
    [[nodiscard]] auto start() requires isClientBidiStream || isServerClientStream || isServerBidiStream
    {
        return start_op();
    }

    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto start_for(response_type& res) requires isClientClientStream
    {
        return start_op(&res);
    }


    // for client: recv status from server or lib
    // for server: send ok status.
    // result type: expected<void, grpc::StatusCode for client / grpc_errc for server>
    [[nodiscard]] auto finish() requires(! isClientUnary && ! isClientClientStream)
    {
        if constexpr(isClient) return finish_op();
        else                   return finish_op(grpc::Status::OK);
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto finish_with_response(auto&& res) requires isServerUnary || isServerClientStream
    {
        return finish_op(DSK_FORWARD(res), grpc::Status::OK);
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto finish_with_response(auto&& res, grpc::WriteOptions const& opts = {}) requires isServerServerStream
                                                                                                   || isServerBidiStream
    {
        return make_server_op<grpc_errc::server_call_finish_failed>
        (
            [this, res = wrap_lref_fwd_other(DSK_FORWARD(res)), opts](void* tag) mutable
            {
                _d.dgt.WriteAndFinish(unwrap_ref_or_move(res), opts, grpc::Status::OK, tag);
            }
        );
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto finish_with_err(auto&& err) requires isServer
    {
        DSK_ASSERT(! err.ok());

        if constexpr(isServerUnary || isServerClientStream)
        {
            return make_server_op<grpc_errc::server_call_finish_failed>
            (
                [this, err = DSK_FORWARD(err)](void* tag) mutable
                {
                    _d.dgt.FinishWithError(mut_move(err), tag);
                }
            );
        }
        else
        {
            return finish_op(DSK_FORWARD(err));
        }
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto finish_with_err(grpc::StatusCode err, _char_str_ auto&&... msgs) requires isServer
    {
        return finish_with_err(grpc::Status(err, DSK_FORWARD(msgs)...));
    }


    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto recv_initial_metadata() requires(isClient && ! isClientUnary)
    {
        return make_client_op([this](void* tag)
        {
            _d.dgt->ReadInitialMetadata(tag);
        });
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto send_initial_metadata() requires isServer
    {
        return make_server_op<grpc_errc::send_initial_metadata_failed>
        (
            [this](void* tag)
            {
                _d.dgt.SendInitialMetadata(tag);
            }
        );
    }


    // NOTE:
    //    For ClientBidiStream and Server: read/write/writes_done always has_val.
    //       For ClientBidiStream:
    //          On false val, the actual status will be received after finish(),
    //          because finish() can only be called when these methods aren't outstanding.
    //       For Server:
    //          On false val, it may be caused by client writes_done or error, but you
    //          can't tell the difference. The only thing you can do is call finish[_xxx]();

    // result type: expected<bool, grpc::StatusCode for client / grpc_errc for server>
    template<bool Seq = false>
    [[nodiscard]] auto read(auto& r) requires isClientServerStream || isClientBidiStream
                                           || isServerClientStream || isServerBidiStream
    {
        return make_act_op
        <
            {},
            bool,
            (isClientBidiStream && ! Seq) ? client_finish_skip : client_finish_non_ok
        >
        (
            [this, &r](void* tag)
            {
                _d.dref().Read(&r, tag);
            }
        );
    }

    // result type: Client: expected<optional<response_type>, grpc::StatusCode>
    //              Server: expected<optional<request_type>, grpc_errc>
    template<bool Seq = false>
    [[nodiscard]] auto read() requires isClientServerStream || isClientBidiStream
                                    || isServerClientStream || isServerBidiStream
    {
        using r_type = optional<std::conditional_t<isClient, response_type, request_type>>;

        return make_act_op
        <
            {},
            r_type,
            (isClientBidiStream && ! Seq) ? client_finish_skip : client_finish_non_ok
        >
        (
            [](data_t& d, r_type& r, void* tag)
            {
                d.dref().Read(std::addressof(r.emplace()), tag);
            }
        );
    }

    // result type: ClientBidiStream: expected<bool, grpc::StatusCode>
    //              Other: expected<void, grpc::StatusCode for client / grpc_errc for server>
    template<bool Seq = false>
    [[nodiscard]] auto write(auto&& r, grpc::WriteOptions const& opts = {}) requires isClientClientStream || isClientBidiStream
                                                                                  || isServerServerStream || isServerBidiStream
    {
        return make_act_op
        <
            grpc_errc::server_call_write_failed,
            std::conditional_t<(isClientBidiStream && ! Seq), bool, void>,
            (isClientBidiStream && ! Seq) ? client_finish_skip : client_finish_non_ok
        >
        (
            [this, r = wrap_lref_fwd_other(DSK_FORWARD(r)), opts](void* tag) mutable
            {
                _d.dref().Write(unwrap_ref_or_move(r), opts, tag);
            }
        );
    }

    // NOTE: should NOT be used with wirte(r, WriteOptions.set_last_message()).
    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto writes_done() requires isClientClientStream || isClientBidiStream
    {
        return make_client_op
        <
            isClientClientStream ? client_finish_forced : client_finish_skip
        >
        (
            [this](void* tag)
            {
                _d.dgt->WritesDone(tag);
            }
        );
    }

    ////// sequential read/write for ClientBidiStream:
    // seq_read() and seq_write() cannot be called concurrently.
    // these methods will call finish() if failed, so `DSK_TRY rpc.seq_xxx()` is well defined.

    // result type: expected<bool, grpc::StatusCode>
    [[nodiscard]] auto seq_read(response_type& res) requires isClientBidiStream
    {
        return read<true>(res);
    }

    // result type: expected<optional<response_type>, grpc::StatusCode>
    [[nodiscard]] auto seq_read() requires isClientBidiStream
    {
        return read<true>();
    }

    // result type: expected<void, grpc::StatusCode>
    [[nodiscard]] auto seq_write(auto&& req, grpc::WriteOptions const& opts = {}) requires isClientBidiStream
    {
        return write<true>(DSK_FORWARD(req), opts);
    }

    ////// unary over bidi stream

    auto get_response(auto&& req, response_type& res) requires isClientBidiStream
    {
        return [](auto& rpc, DSK_LREF_OR_VAL_T(req) req, response_type& res) -> task<void>
        {
            DSK_TRY rpc.seq_write(DSK_MOVE_NON_LREF(req));

            if(! DSK_TRY rpc.seq_read(res))
            {
                DSK_TRY rpc.writes_done();
                DSK_TRY rpc.finish();
                DSK_THROW(grpc_errc::client_bidi_unary_get_no_res);
            }

            DSK_RETURN();
        }
        (*this, DSK_FORWARD(req), res);
    }

    auto get_response(auto&& req) requires isClientBidiStream
    {
        return [](auto& rpc, DSK_LREF_OR_VAL_T(req) req) -> task<response_type>
        {
            DSK_TRY rpc.seq_write(DSK_MOVE_NON_LREF(req));

            response_type res;

            if(! DSK_TRY rpc.seq_read(res))
            {
                DSK_TRY rpc.writes_done();
                DSK_TRY rpc.finish();
                DSK_THROW(grpc_errc::client_bidi_unary_get_no_res);
            }

            DSK_RETURN_MOVED(res);
        }
        (*this, DSK_FORWARD(req));
    }


    task<void> get_request(request_type& req) requires isServerBidiStream
    {
        if(! DSK_TRY read(req))
        {
            DSK_TRY finish_with_err(grpc::FAILED_PRECONDITION, "bidi stream unary get no request");
            DSK_THROW(grpc_errc::server_bidi_unary_get_no_req);
        }

        DSK_RETURN();
    }

    task<request_type> get_request() requires isServerBidiStream
    {
        request_type req;

        if(! DSK_TRY read(req))
        {
            DSK_TRY finish_with_err(grpc::FAILED_PRECONDITION, "bidi stream unary get no request");
            DSK_THROW(grpc_errc::server_bidi_unary_get_no_req);
        }

        DSK_RETURN_MOVED(req);
    }

    // result type: expected<void, grpc_errc>
    [[nodiscard]] auto send_response(auto&& res, grpc::WriteOptions const& opts = {}) requires isServerBidiStream
    {
        return write(DSK_FORWARD(res), opts);
    }

};


using grpc_generic_client_unary_rpc = grpc_rpc
<
    static_cast<
        std::unique_ptr<grpc::ClientAsyncResponseReader<grpc::ByteBuffer>>(grpc::GenericStub::*)(
            grpc::ClientContext*, std::string const&, grpc::ByteBuffer const&, grpc::CompletionQueue*)
    >(&grpc::GenericStub::PrepareUnaryCall)
>;

using grpc_generic_client_bidi_stream_rpc = grpc_rpc<&grpc::GenericStub::PrepareCall>;
using grpc_generic_server_bidi_stream_rpc = grpc_rpc<&grpc::AsyncGenericService::RequestCall>;


template<_tuple_like_ Services, class Cq>
template<auto Method>
task<> grpc_srv_t<Services, Cq>::create_task(auto f)
{
    auto opGrp = DSK_WAIT make_async_op_group();

    for(;;)
    {
        using rpc_t = grpc_rpc<Method, Cq>;

        auto rpc = allocate_unique<rpc_t>(allocator_type(), *this);

        if constexpr(rpc_t::isServerUnary || rpc_t::isServerServerStream)
        {
            auto req = DSK_TRY rpc->get_request();
            opGrp.add_and_initiate(f(mut_move(rpc), mut_move(req)));
        }
        else
        {
            DSK_TRY rpc->start();
            opGrp.add_and_initiate(f(mut_move(rpc)));
        }
    }

    DSK_RETURN();
}


// NOTE: rpc can't be reused
// 
// client unary:
// 
//      DSK_TRY rpc.get_response(req, res);
//      // or: auto res = DSK_TRY rpc.get_response(req);
//
// server unary:
// 
//      DSK_TRY rpc.get_request(req);
//      // or: auto req = DSK_TRY rpc.get_request();
//      
//      if(ok) DSK_TRY rpc.finish_with_response(res);
//      else   DSK_TRY rpc.finish_with_err(err);
//      
// client client stream:
// 
//      DSK_TRY rpc.start_for(res);
//      
//      while(has_more_requests)
//      {
//          DSK_TRY rpc.write(req);
//      }
//      
//      DSK_TRY rpc.writes_done();
//      use(res);
//    
// server client stream:
// 
//      DSK_TRY rpc.start();
//      
//      while(
//          DSK_TRY rpc.read(req)
//          // or
//          auto req = DSK_TRY rpc.read())
//      {
//          use(req);
//      }
//      
//      if(ok) rpc.finish_with_response(res);
//      else   rpc.finish_with_err(err);
//      
// client server stream:
// 
//      DSK_TRY rpc.send_request(req);
//      
//      while(
//          DSK_TRY rpc.read(res)
//          // or
//          auto res = DSK_TRY rpc.read())
//      {
//          use(res or *res);
//      }
//      
// server server stream:
//
//      DSK_TRY rpc.get_request(req);
//      // or: auto req = DSK_TRY rpc.get_request();
//      
//      while(has_more_response)
//      {
//          DSK_TRY rpc.write(res);
//      }
//      
//      if(ok) rpc.finish();
//      else   rpc.finish_with_err(err);
// 
// client bidi stream:
// 
//      DSK_TRY rpc.start();
// 
//      auto[writeResult, readResult] = DSK_TRY until_all_done
//      (
//          // write task
//          [&]() -> task<>
//          {
//              // DSK_TRY resume_on(write_scheduler);
//              
//              while(has_more_requests)
//              {
//                  if(! DSK_TRY rpc.write(req))
//                      DSK_RETURN();
//              }
// 
//              DSK_TRY rpc.writes_done();
//              DSK_RETURN();
//          }(),
//          // read task
//          [&]() -> task<>
//          {
//              // DSK_TRY resume_on(read_scheduler);
//              
//              while(DSK_TRY rpc.read(res)) // or auto res = DSK_TRY rpc.read()
//              {
//                  use(res);// or *res
//              }
// 
//              DSK_RETURN();
//          }()
//      );
// 
//      DSK_TRY rpc.finish();
//      
//      // or if you want to get detailed status
//      //if(has_err(DSK_WAIT rpc.finish()))
//      //{
//      //    check(rpc.status());
//      //}
// 
// server bidi stream:
// 
//      DSK_TRY rpc.start();
// 
//      auto[writeResult, readResult] = DSK_TRY until_all_done
//      (
//          // write task
//          [&]() -> task<>
//          {
//              // DSK_TRY resume_on(write_scheduler);
//              
//              while(has_more_response)
//              {
//                  DSK_TRY rpc.write(res);
//              }

//              DSK_RETURN();
//          }(),
//          // read task
//          [&]() -> task<>
//          {
//              // DSK_TRY resume_on(read_scheduler);
//              
//              while(DSK_TRY rpc.read(req)) // or auto req = DSK_TRY rpc.read()
//              {
//                  use(req);// or *req
//              }
// 
//              DSK_RETURN();
//          }()
//      );
// 
//      if(ok) rpc.finish();
//      else   rpc.finish_with_err(err);


} // namespace dsk
