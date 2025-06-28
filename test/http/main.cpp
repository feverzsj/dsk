#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/http/conn.hpp>
#include <dsk/http/content_type.hpp>
#include <dsk/http/file_handler.hpp>
#include <dsk/http/mime.hpp>
#include <dsk/http/msg.hpp>
#include <dsk/http/parse.hpp>
#include <dsk/http/range_header.hpp>


TEST_CASE("http")
{
    using namespace dsk;

    SUBCASE("content_type")
    {
        constexpr auto parse = [](char const* s,
                                  char const* mime = "", char const* charset = "", char const* boundary = "") -> bool
        {
            http_content_type c;
            c.parse(s);

            return c.mime     == mime
                && c.charset  == charset
                && c.boundary == boundary;
        };

        CHECK(parse("text/html"                                             , "text/html"));
        CHECK(parse("text/html;"                                            , "text/html"));
        CHECK(parse("text/html; charset=utf-8"                              , "text/html", "utf-8"));
        CHECK(parse("text/html; charset =utf-8"                             , "text/html"));
        CHECK(parse("text/html; charset= utf-8"                             , "text/html", "utf-8"));
        CHECK(parse("text/html; charset=utf-8 "                             , "text/html", "utf-8"));
        CHECK(parse("text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs\""   , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
        CHECK(parse("text/html; boundary =\"WebKit-ada-df-dsf-adsfadsfs\""  , "text/html"));
        CHECK(parse("text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\""  , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
        CHECK(parse("text/html; boundary= \"WebKit-ada-df-dsf-adsfadsfs\"  ", "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
        CHECK(parse("text/html; boundary=\"WebKit-ada-df-dsf-adsfadsfs  \"" , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
        CHECK(parse("text/html; boundary=WebKit-ada-df-dsf-adsfadsfs"       , "text/html", "", "WebKit-ada-df-dsf-adsfadsfs"));
        CHECK(parse("text/html; charset"                                    , "text/html"));
        CHECK(parse("text/html; charset="                                   , "text/html"));
        CHECK(parse("text/html; charset= "                                  , "text/html"));
        CHECK(parse("text/html; charset= ;"                                 , "text/html"));
        CHECK(parse("text/html; charset=\"\""                               , "text/html"));
        CHECK(parse("text/html; charset=\" \""                              , "text/html"));
        CHECK(parse("text/html; charset=\" foo \""                          , "text/html", "foo"));
        CHECK(parse("text/html; charset=foo; charset=utf-8"                 , "text/html", "foo"));
        CHECK(parse("text/html; charset; charset=; charset=utf-8"           , "text/html", "utf-8"));
        CHECK(parse("text/html; charset=utf-8; charset=; charset"           , "text/html", "utf-8"));
        CHECK(parse("text/html; boundary=foo; boundary=bar"                 , "text/html", "", "foo"));
        CHECK(parse("text/html; \"; \"\"; charset=utf-8"                    , "text/html", "utf-8"));
        CHECK(parse("text/html; charset=u\"tf-8\""                          , "text/html", "u\"tf-8\""));
        CHECK(parse("text/html; charset=\"utf-8\""                          , "text/html", "utf-8"));
        CHECK(parse("text/html; charset=\"utf-8"                            , "text/html", "utf-8"));
        CHECK(parse("text/html; charset=\"\\utf\\-\\8\""                    , "text/html", "utf-8"));
        //CHECK(parse("text/html; charset=\"\\\\\\\"\\"                       , "text/html", "\\\"\\"));
        CHECK(parse("text/html; charset=\";charset=utf-8;\""                , "text/html", ";charset=utf-8;"));
        CHECK(parse("text/html; charset= \"utf-8\""                         , "text/html", "utf-8"));
        CHECK(parse("text/html; charset='utf-8'"                            , "text/html", "'utf-8'"));

    } // SUBCASE("content_type")

    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }

    SUBCASE("server-client")
    {
        constexpr int nTask = 6;
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_done
        (
            // server
            [&]() -> task<>
            {
                tcp_acceptor acceptor(tcp_endpoint(tcp_v4(), 2626));

                auto opGrp = DSK_WAIT make_async_op_group();

                for(int i = 0; i < nTask; ++i)
                {
                    opGrp.add_and_initiate([](auto conn) -> task<>
                    {
                        for(int i = 0; i < nCall; ++i)
                        {
                            auto req = DSK_TRY conn.read_request();

                            CHECK(i == DSK_TRY_SYNC str_to<int>(req["test_hdr"]));
                            CHECK(i + 1 == DSK_TRY_SYNC str_to<int>(req.body()));
                            CHECK(req.keep_alive());

                            http_response res(http_status::ok, req.version());
                            res.keep_alive(true);
                            res.set("test_hdr", str_view(stringify(i + 2)));
                            res.body() = str_view(stringify(i + 3));
                            res.prepare_payload();

                            DSK_TRY conn.write(res);
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
                
                for(int i = 0; i < nTask; ++i)
                {
                    opGrp.add_and_initiate([]() -> task<>
                    {
                        http_conn conn;
                        //DSK_TRY conn.connect(DSK_TRY_SYNC make_ip_addr("127.0.0.1"), 2626);
                        DSK_TRY conn.connect(ip_addr_v4({127,0,0,1}), 2626);

                        for(int i = 0; i < nCall; ++i)
                        {
                            http_request req;
                            req.method(http_verb::post);
                            req.target("/");
                            req.keep_alive(true);
                            req.insert("test_hdr", stringify(i));
                            req.body() = stringify(i + 1);
                            req.prepare_payload();

                            auto res = DSK_TRY conn.read_response(req);

                            CHECK(res.keep_alive());
                            CHECK(i + 2 == DSK_TRY_SYNC str_to<int>(res["test_hdr"]));
                            CHECK(i + 3 == DSK_TRY_SYNC str_to<int>(res.body()));
                        }

                        DSK_RETURN();
                    }());
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("tcp")

} // TEST_CASE("http")
