#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/redis/req.hpp>
#include <dsk/redis/cmds.hpp>
#include <dsk/redis/conn.hpp>
#include <map>
#include <vector>


TEST_CASE("redis")
{
    using namespace dsk;
    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }

    redis_config cfg = 
    {
        .addr = {"localhost", "6379"},
        .password = "",
        .clientname = "test_redis"
    };

    SUBCASE("ping")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                redis_conn conn;
                conn.start(cfg, redis_logger(redis_logger::level::warning));
                
                auto rr = DSK_TRY conn.exec<redis_response<string>>(
                    redis_cmds::ping("Hello world")
                );

                CHECK(get_elm<0>(rr).value() == "Hello world");

                auto tr = DSK_TRY conn.exec<tuple<string>>(
                    redis_cmds::ping("Hello world")
                );

                CHECK(get_elm<0>(tr) == "Hello world");

                auto r = DSK_TRY conn.exec<string>(
                    redis_cmds::ping("Hello world")
                );

                CHECK(r == "Hello world");

                string str;
                DSK_TRY conn.exec_get(
                    str,
                    redis_cmds::ping("Hello world")
                );

                CHECK(str == "Hello world");

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("ping")


    SUBCASE("container")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                redis_conn conn;
                conn.start(cfg, redis_logger(redis_logger::level::warning));
                
                DSK_TRY conn.exec(
                    redis_cmds::del("rpush-key", "hset-key")
                );

                std::vector<int>         vec{1, 2, 3, 4, 5, 6};
                std::map<string, string> map{{"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}};

                DSK_TRY conn.exec(
                    redis_cmds::rpush("rpush-key", vec),
                    redis_cmds::hset("hset-key", map)
                );

                auto ov = DSK_TRY conn.exec<optional<decltype(vec)>>(
                    redis_cmds::lrange("rpush-key", 0, -1)
                );
                CHECK(*ov == vec);

                auto vm = DSK_TRY conn.exec<tuple<decltype(vec), decltype(map)>>(
                    redis_cmds::lrange("rpush-key", 0, -1),
                    redis_cmds::hgetall("hset-key")
                );
                CHECK(get_elm<0>(vm) == vec);
                CHECK(get_elm<1>(vm) == map);

                decltype(map) rm;
                DSK_TRY conn.exec_get(
                    tuple(redis::ignore, std::ref(rm)),
                    redis_cmds::lrange("rpush-key", 0, -1),
                    redis_cmds::hgetall("hset-key")
                );
                CHECK(rm == map);

                auto txn = DSK_TRY conn.exec<
                    tuple<// multi        lrange         hgetall 
                        redis_ignore_t, redis_ignore_t, redis_ignore_t,
                            // exec
                        tuple<decltype(vec), decltype(map)>>>
                (
                    redis_cmds::multi,
                    redis_cmds::lrange("rpush-key", 0, -1),
                    redis_cmds::hgetall("hset-key"),
                    redis_cmds::exec
                );
                CHECK(txn[idx_c<3>][idx_c<0>] == vec);
                CHECK(txn[idx_c<3>][idx_c<1>] == map);

                decltype(vec) tv;
                decltype(map) tm;
                DSK_TRY conn.exec_get(
                    tuple(// multi        lrange         hgetall 
                        redis::ignore, redis::ignore, redis::ignore,
                            // exec
                        fwd_as_tuple(tv, tm)
                    ),
                    redis_cmds::multi,
                    redis_cmds::lrange("rpush-key", 0, -1),
                    redis_cmds::hgetall("hset-key"),
                    redis_cmds::exec
                );
                CHECK(tv == vec);
                CHECK(tm == map);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("container")


    SUBCASE("pub-sub")
    {
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_done
        (
            // subscriber
            [&]() -> task<>
            {
                redis_conn conn;
                conn.start(cfg, redis_logger(redis_logger::level::warning));

                redis_generic_response res;
                //redis_response<redis_ignore_t, redis_ignore_t, redis_ignore_t, int> res;
                conn.set_receive_response(res);

                int i = 0;
                while(conn.will_reconnect())
                {
                    DSK_TRY conn.exec(redis_cmds::subscribe("channel"));

                    for(bool firstRcv = true; ; firstRcv = false)
                    {
                        auto r = DSK_WAIT conn.receive();

                        if(has_err(r))
                        {
                            break; // Connection lost, try reconnecting to channels.
                        }

                        auto& nodes = res.value();

                        if(! firstRcv)
                        {
                            CHECK(nodes.at(3).value == std::to_string(i));
                            //CHECK(get_elm<3>(res).value() == i);

                            if(++i == nCall)
                            {
                                DSK_RETURN();
                            }
                        }

                        nodes.clear();
                    }
                }

                DSK_RETURN();
            }(),
            // publisher
            [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::seconds(1)); // make sure subscriber is ready, as redis pub/sub doesn't queue messages.

                redis_conn conn;
                conn.start(cfg, redis_logger(redis_logger::level::warning));

                for(int i = 0; i < nCall; ++i)
                {
                    auto res = DSK_TRY conn.exec<redis_response<int>>(redis_cmds::publish("channel", i));
                    CHECK(get_elm<0>(res).value() == 1); // successfully publish to 1 subscriber
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("pub-sub")


} // TEST_CASE("redis")
