#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/util/from_str.hpp>
#include <dsk/grpc/rpc.hpp>
#include <dsk/grpc/srv.hpp>
#include <dsk/grpc/timer.hpp>
#include <dsk/grpc/channel_pool.hpp>

#include "example.grpc.pb.h"


TEST_CASE("grpc")
{
    using namespace dsk;

    using request_type = Request;
    using response_type = Response;
    using stub_type = Example::Stub;
    using service_type = Example::AsyncService;

    // NOTE: grpc objects cannot be inited in global or namespace.

    grpc_channel_pool chPool({.target = "localhost:50051", .channels = 2});

    stub_type stub(chPool.get());
    
    auto server = grpc_srv
    <
        service_type,
        grpc::AsyncGenericService
    >(
        [](grpc::ServerBuilder& builder)
        {
            builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
        },
        2, // nCq
        threads_per_cq(3),
        start_now
    );


    SUBCASE("unary")
    {
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                auto& cq = server.cq(); // use same cq

                grpc_timer timer(cq);

                for(int i = 0; i < nCall; ++i)
                {
                    grpc_rpc<&stub_type::AsyncUnary> rpc(stub, cq);

                    //rpc.context().set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

                    request_type req;
                    req.set_integer(i);
                    response_type res;
                    DSK_TRY rpc.get_response(req, res);

                    CHECK(res.integer() == i);

                    DSK_TRY timer.sleep_for(std::chrono::milliseconds(26));
                }

                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                for(;;)
                {
                    grpc_rpc<&service_type::RequestUnary> rpc(server);

                    request_type req = DSK_TRY rpc.get_request();
                    response_type res;
                    res.set_integer(req.integer());
                    DSK_TRY rpc.finish_with_response(res);

                    if(req.integer() + 1  >= nCall)
                        break;
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("unary")


    SUBCASE("client_stream")
    {
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                auto& cq = server.cq(); // use same cq

                grpc_timer timer(cq);

                grpc_rpc<&stub_type::AsyncClientStreaming> rpc(stub, cq);

                response_type res;
                DSK_TRY rpc.start_for(res);

                for(int i = 0; i < nCall; ++i)
                {
                    request_type req;
                    req.set_integer(i);
                    DSK_TRY rpc.write(req);
                    DSK_TRY timer.sleep_for(std::chrono::milliseconds(26));
                }

                DSK_TRY rpc.writes_done();

                CHECK(res.integer() == nCall - 1);

                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                grpc_rpc<&service_type::RequestClientStreaming> rpc(server);

                DSK_TRY rpc.start();

                request_type req;
                req.set_integer(-1); // error value if read failed on first call.

                while(DSK_TRY rpc.read(req))
                    ;

                response_type res;
                res.set_integer(req.integer());
                DSK_TRY rpc.finish_with_response(res);

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("client_stream")


    SUBCASE("server_stream")
    {
        constexpr int nCall = 66;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                grpc_rpc<&stub_type::AsyncServerStreaming> rpc(stub, server.cq());

                request_type req;
                req.set_integer(nCall);
                DSK_TRY rpc.send_request(req);

                response_type res;
                while(DSK_TRY rpc.read(res))
                    ;

                CHECK(res.integer() == nCall - 1);

                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                grpc_rpc<&service_type::RequestServerStreaming> rpc(server);

                auto req = DSK_TRY rpc.get_request();

                CHECK(req.integer() == nCall);

                for(int i = 0; i < nCall; ++i)
                {
                    response_type res;
                    res.set_integer(i);
                    DSK_TRY rpc.write(res);
                }

                DSK_TRY rpc.finish();

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("server_stream")


    SUBCASE("bidi_stream")
    {
        constexpr int nCall = 66;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                grpc_rpc<&stub_type::AsyncBidirectionalStreaming> rpc(stub, server.cq());

                DSK_TRY rpc.start();

                auto[writeResult, readResult] = DSK_TRY until_all_done
                (
                    // write
                    [&]() -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            request_type req;
                            req.set_integer(i);
                            if(! DSK_TRY rpc.write(req))
                                DSK_RETURN();
                        }

                        DSK_TRY rpc.writes_done();
                        DSK_RETURN();
                    }(),
                    // read
                    [&]() -> task<>
                    {                        
                        response_type res;
                        while(DSK_TRY rpc.read(res))
                            ;

                        CHECK(res.integer() == nCall - 1);
                
                        DSK_RETURN();
                    }()
                );

                CHECK(! has_err(writeResult));
                CHECK(! has_err(readResult));

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                grpc_rpc<&service_type::RequestBidirectionalStreaming> rpc(server);

                DSK_TRY rpc.start();

                auto[writeResult, readResult] = DSK_TRY until_all_done
                (
                    // write
                    [&]() -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            response_type res;
                            res.set_integer(i);
                            DSK_TRY rpc.write(res);
                        }

                        DSK_RETURN();
                    }(),
                    // read
                    [&]() -> task<>
                    {                        
                        request_type req;
                        while(DSK_TRY rpc.read(req))
                            ;

                        CHECK(req.integer() == nCall - 1);
                
                        DSK_RETURN();
                    }()
                );

                CHECK(! has_err(writeResult));
                CHECK(! has_err(readResult));

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("bidi_stream")


    SUBCASE("unary_over_bidi_stream")
    {
        constexpr int nCall = 66;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                grpc_rpc<&stub_type::AsyncBidirectionalStreaming> rpc(stub, server.cq());
                DSK_TRY rpc.start();

                for(int i = 0; i < nCall; ++i)
                {
                    request_type req;
                    req.set_integer(i);
                    response_type res;
                    DSK_TRY rpc.get_response(req, res);

                    CHECK(res.integer() == i);
                }

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                grpc_rpc<&service_type::RequestBidirectionalStreaming> rpc(server);
                DSK_TRY rpc.start();

                for(;;)
                {
                    request_type req = DSK_TRY rpc.get_request();
                    response_type res;
                    res.set_integer(req.integer());
                    DSK_TRY rpc.send_response(res);

                    if(req.integer() + 1  >= nCall)
                        break;
                }

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("unary_over_bidi_stream")


    SUBCASE("generic_unary")
    {
        constexpr int nCall = 66;
        std::string name = "generic_unary_call";

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                grpc::GenericStub stub(chPool.get());

                for(int i = 0; i < nCall; ++i)
                {
                    grpc_generic_client_unary_rpc rpc(name, stub, server.cq());

                    grpc::ByteBuffer res = DSK_TRY rpc.get_response(to_grpc_buf_coll(stringify(i)));

                    grpc::Slice sl = DSK_TRY_SYNC uncompress_and_merge(res);
                    CHECK(str_view(sl) == str_view(stringify(i)));
                }

                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                for(;;)
                {
                    grpc_generic_server_bidi_stream_rpc rpc(server);
                    DSK_TRY rpc.start();

                    CHECK(rpc.context().method() == name);

                    grpc::ByteBuffer req = DSK_TRY rpc.get_request();
                    grpc::Slice sl = DSK_TRY_SYNC uncompress_and_merge(req);

                    int res = DSK_TRY_SYNC str_to<int>(str_view(sl));
                    DSK_TRY rpc.finish_with_response(to_grpc_buf_coll(stringify(res)));

                    if(res + 1  >= nCall)
                        break;
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("generic_unary")


    SUBCASE("generic_bidi_stream")
    {
        constexpr int nCall = 66;
        std::string name = "generic_bidi_stream_call";

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                grpc::GenericStub stub(chPool.get());

                grpc_generic_client_bidi_stream_rpc rpc(name, stub, server.cq());

                DSK_TRY rpc.start();

                auto[writeResult, readResult] = DSK_TRY until_all_done
                (
                    // write
                    [&]() -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            if(! DSK_TRY rpc.write(to_grpc_buf_coll(stringify(i))))
                                DSK_RETURN();
                        }

                        DSK_TRY rpc.writes_done();
                        DSK_RETURN();
                    }(),
                    // read
                    [&]() -> task<>
                    {                        
                        grpc::ByteBuffer res;
                        while(DSK_TRY rpc.read(res))
                            ;

                        grpc::Slice sl = DSK_TRY_SYNC uncompress_and_merge(res);
                        CHECK(str_view(sl) == str_view(stringify(nCall - 1)));
                
                        DSK_RETURN();
                    }()
                );

                CHECK(! has_err(writeResult));
                CHECK(! has_err(readResult));

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                grpc_generic_server_bidi_stream_rpc rpc(server);

                DSK_TRY rpc.start();
                
                CHECK(rpc.context().method() == name);

                auto[writeResult, readResult] = DSK_TRY until_all_done
                (
                    // write
                    [&]() -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            DSK_TRY rpc.write(to_grpc_buf_coll(stringify(i)));
                        }

                        DSK_RETURN();
                    }(),
                    // read
                    [&]() -> task<>
                    {                        
                        grpc::ByteBuffer req;
                        while(DSK_TRY rpc.read(req))
                            ;

                        grpc::Slice sl = DSK_TRY_SYNC uncompress_and_merge(req);
                        CHECK(str_view(sl) == str_view(stringify(nCall - 1)));
                
                        DSK_RETURN();
                    }()
                );

                CHECK(! has_err(writeResult));
                CHECK(! has_err(readResult));

                DSK_TRY rpc.finish();
                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("generic_bidi_stream")


    SUBCASE("unary_server_task")
    {
        constexpr int nCall = 66;

        // stop_source is not necessary here, just a show case of override_ctx().
        std::stop_source srvSs;

        auto r = sync_wait(until_all_done
        (
            // client
            [&]() -> task<>
            {
                for(int i = 0; i < nCall; ++i)
                {
                    grpc_rpc<&stub_type::AsyncUnary> rpc(stub, server.cq());

                    request_type req;
                    req.set_integer(i);
                    response_type res;
                    DSK_TRY rpc.get_response(req, res);

                    CHECK(res.integer() == i);
                }

                CHECK(srvSs.request_stop());
                server.stop();
                DSK_RETURN();
            }(),
            // server
            override_ctx
            (
                server.create_task<&service_type::RequestUnary>([](auto rpc, auto req) -> task<>
                {
                    response_type res;
                    res.set_integer(req.integer());
                    DSK_TRY rpc->finish_with_response(res);
                    DSK_RETURN();
                }),
                srvSs
            )
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(is_err(get_elm<1>(get_val(r)), grpc_errc::server_call_start_failed));

    }// SUBCASE("unary_server_task")


     /// !!! server shutdown in unary_server_task
     /// !!! server shutdown in unary_server_task
     /// !!! server shutdown in unary_server_task


} // TEST_CASE("grpc")
