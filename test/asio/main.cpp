#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/asio/ip.hpp>
#include <dsk/asio/tcp.hpp>
#include <dsk/asio/udp.hpp>
#include <dsk/asio/ssl.hpp>
#include <dsk/asio/write.hpp>
#include <dsk/asio/write_at.hpp>
#include <dsk/asio/read.hpp>
#include <dsk/asio/read_at.hpp>
#include <dsk/asio/connect.hpp>
#ifdef BOOST_ASIO_HAS_FILE
#include <dsk/asio/file.hpp>
#endif


TEST_CASE("asio")
{
    using namespace dsk;
    using std::chrono::milliseconds;
    using std::chrono::microseconds;
    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }


    SUBCASE("timer")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                constexpr auto expiryDur = milliseconds(26);

                using timer_type = steady_timer;

                auto t = timer_type::now();
                DSK_TRY wait_for<timer_type>(expiryDur);
                auto dur = timer_type::now() - t;
                CHECK(expiryDur <= dur);

                t = timer_type::now();
                DSK_TRY wait_until<timer_type>(t + expiryDur);
                dur = timer_type::now() - t;
                CHECK(expiryDur <= dur);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("timer")


    SUBCASE("timed_op")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                constexpr auto expiryDur = milliseconds(26);

                using timer_type = steady_timer;

                auto t = timer_type::now();
                auto r = DSK_WAIT wait_for<timer_type>(expiryDur, wait_for<timer_type>(milliseconds(266)));
                CHECK(is_err(r, errc::timeout));
                auto dur = timer_type::now() - t;
                CHECK(expiryDur <= dur);

                t = timer_type::now();
                r = DSK_WAIT wait_until<timer_type>(t + milliseconds(266), wait_for<timer_type>(expiryDur));
                CHECK(! has_err(r));
                dur = timer_type::now() - t;
                CHECK(expiryDur <= dur);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("timed_op")


    SUBCASE("tcp")
    {
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_succeeded
        (
            // server
            [&]() -> task<>
            {
                tcp_acceptor acceptor(tcp_endpoint(tcp_v4(), 6262)); // bind to 0.0.0.0:6262
                auto socket = DSK_TRY acceptor.accept();

                size_t n = 0;

                for(;;)
                {
                    int req = -1;
                    
                    n = DSK_TRY socket.read(as_buf_of<char>(req));
                    CHECK(n == sizeof(req));
                    CHECK(req < nCall);
                    n = DSK_TRY socket.write(as_buf_of<char>(req));
                    CHECK(n == sizeof(req));

                    if(req + 1  >= nCall)
                        break;
                }

                DSK_RETURN();
            }(),
            // client
            [&]() -> task<>
            {
                DSK_TRY wait_for(milliseconds(500));

                tcp_resolver resolver;
                auto endpoints = DSK_TRY resolver.resolve("127.0.0.1", "6262");

                tcp_socket socket;
                DSK_TRY socket.connect(endpoints);

                //tcp_socket socket;
                //DSK_TRY socket.connect(ip_addr_v4({127,0,0,1}), 6262);
                
                size_t n = 0;

                for(int i = 0; i < nCall; ++i)
                {
                    int res = -1;

                    n = DSK_TRY socket.write(as_buf_of<char>(i));
                    CHECK(n == sizeof(i));
                    n = DSK_TRY socket.read(as_buf_of<char>(res));
                    CHECK(n == sizeof(res));

                    CHECK(res == i);
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));

    }// SUBCASE("tcp")


    SUBCASE("udp")
    {
        constexpr int nCall = 26;
        udp_endpoint srvEndpoint(ip_addr_v4({127,0,0,1}), 2626);

        auto r = sync_wait(until_all_succeeded
        (
            // server
            [&]() -> task<>
            {
                udp_socket socket(srvEndpoint);

                size_t n = 0;

                for(;;)
                {
                    int req = -1;
                    udp_endpoint peer;

                    n = DSK_TRY socket.receive_from(as_buf_of<char>(req), peer);
                    CHECK(n == sizeof(req));
                    CHECK(req < nCall);
                    n = DSK_TRY socket.send_to(as_buf_of<char>(req), peer);
                    CHECK(n == sizeof(req));

                    if(req + 1  >= nCall)
                        break;
                }

                DSK_RETURN();
            }(),
            // client
            [&]() -> task<>
            {
                DSK_TRY wait_for(milliseconds(500));

                udp_socket socket;
                DSK_TRY_SYNC socket.open(srvEndpoint.protocol());

                size_t n = 0;

                for(int i = 0; i < nCall; ++i)
                {
                    int res = -1;
                    udp_endpoint peer;

                    n = DSK_TRY socket.send_to(as_buf_of<char>(i), srvEndpoint);
                    CHECK(n == sizeof(i));
                    n = DSK_TRY socket.receive_from(as_buf_of<char>(res), peer);
                    CHECK(n == sizeof(res));

                    CHECK(res == i);
                }

                DSK_RETURN();
            }()

        ));

        CHECK(! has_err(r));

    }// SUBCASE("udp")


    SUBCASE("ssl")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                ssl_context ctx(ssl_context::sslv23);
                //DSK_TRY_SYNC ctx.set_default_verify_paths();

                tcp_resolver resolver;
                auto endpoints = DSK_TRY resolver.resolve("example.com", "443");

                ssl_stream<tcp_socket> socket(ctx);
                DSK_TRY socket.next_layer().connect(endpoints);

                //DSK_TRY_SYNC socket.next_layer().set_option(asio::ip::tcp::no_delay(true));
                DSK_TRY_SYNC socket.set_verify_mode(asio::ssl::verify_none);
                //DSK_TRY_SYNC socket.set_verify_callback(ssl_host_name_verification("www.example.com"));

                DSK_TRY socket.handshake(ssl_handshake::client);

                DSK_TRY socket.write("GET / HTTP/1.1\r\nHost:www.example.com\r\n\r\n");

                string res;
                DSK_TRY socket.read_until(res, "\r\n");
                CHECK(res.starts_with("HTTP/1.1 200 OK"));

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));

    }// SUBCASE("ssl")

#ifdef BOOST_ASIO_HAS_FILE
    SUBCASE("file")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                constexpr char const* path = "test_asio_file.txt";
                constexpr std::string_view data = "test_asio_file.txt";
                size_t n = 0;
                string buf(data.size(), '\0');

                {
                    stream_file file;
                    DSK_TRY_SYNC file.open(path, file_base::read_write | file_base::create | file_base::truncate);

                    n = DSK_TRY file.write(data);
                    CHECK(n == data.size());
                    DSK_TRY_SYNC file.sync_all();
                    n = DSK_TRY_SYNC file.size();
                    CHECK(n == data.size());

                    DSK_TRY_SYNC file.rewind();
                    n = DSK_TRY file.read(buf);
                    CHECK(n == data.size());
                    CHECK(data == buf);
                }

                {
                    random_access_file file;
                    DSK_TRY_SYNC file.open(path, file_base::read_write);

                    std::ranges::fill(buf, '\0');
                    n = DSK_TRY file.read_at(0, buf);
                    CHECK(n == data.size());
                    CHECK(data == buf);

                    n = DSK_TRY file.write_at(data.size(), data);
                    CHECK(n == data.size());
                    DSK_TRY_SYNC file.sync_all();
                    n = DSK_TRY_SYNC file.size();
                    CHECK(n == data.size()*2);

                    std::ranges::fill(buf, '\0');
                    n = DSK_TRY file.read_at(data.size(), buf);
                    CHECK(n == data.size());
                    CHECK(data == buf);
                }

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("file")

#endif // BOOST_ASIO_HAS_FILE

} // TEST_CASE("asio")
