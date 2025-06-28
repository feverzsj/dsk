#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/util/from_str.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/http/conn.hpp>
#include <dsk/curl/client.hpp>


TEST_CASE("curl")
{
    using namespace dsk;
    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }

    SUBCASE("client")
    {
        constexpr int nTask = 6;
        constexpr int nCall = 26;

        auto r = sync_wait(until_first_done
        (
            // server
            [&]() -> task<>
            {
                tcp_acceptor acceptor(tcp_endpoint(tcp_v4(), 2626));

                auto opGrp = DSK_WAIT make_async_op_group();

                for(;;)
                {
                    opGrp.add_and_initiate([](auto conn) -> task<>
                    {
                        for(;;)
                        {
                            http_request req;
                            DSK_TRY conn.read(req);

                            int j = DSK_TRY_SYNC str_to<int>(req["test_hdr"]);

                            CHECK(j + 1 == DSK_TRY_SYNC str_to<int>(req.body()));

                            http_response res(http_status::ok, req.version());
                            res.keep_alive(true);
                            res.set("test_hdr", str_view(stringify(j + 2)));
                            res.body() = str_view(stringify(j + 3));
                            res.prepare_payload();

                            DSK_TRY conn.write(res);

                            if(! req.keep_alive() || j % 6 == 0)
                                break;
                        }

                        DSK_RETURN();
                    }(DSK_TRY acceptor.accept<http_conn>()));
                }

                DSK_RETURN();
            }(),
            // client
            [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::milliseconds(500));
                
                auto opGrp = DSK_WAIT make_async_op_group();

                curl_client client(nTask);
                
                for(int i = 0; i < nTask; ++i)
                {
                    opGrp.add_and_initiate([](auto cr) -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            http_request req;
                            req.method(http_verb::post);
                            req.target("http://127.0.0.1:2626");
                            req.keep_alive(true);
                            req.insert("test_hdr", stringify(i));
                            req.body() = stringify(i + 1);
                            req.prepare_payload();

                            auto res = DSK_TRY cr->read_response(req);

                            CHECK(res.keep_alive());
                            CHECK(i + 2 == DSK_TRY_SYNC str_to<int>(res["test_hdr"]));
                            CHECK(i + 3 == DSK_TRY_SYNC str_to<int>(res.body()));
                        }

                        DSK_RETURN();
                    }(DSK_TRY client.acquire_request()));
                }

                DSK_TRY opGrp.until_all_done();
                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(get_val(r).index() == 1);
        CHECK(! has_err(std::get<1>(get_val(r))));

    }// SUBCASE("client")

} // TEST_CASE("curl")
