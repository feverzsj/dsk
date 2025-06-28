#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/generator.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/start_on.hpp>
#include <dsk/resume_on.hpp>
#include <dsk/res_pool.hpp>
#include <dsk/res_queue.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/util/atomic.hpp>
#include <dsk/tbb/thread_pool.hpp>
#include <dsk/asio/thread_pool.hpp>
#include <dsk/simple_thread_pool.hpp>
#ifdef BOOST_WINDOWS
    #include <dsk/win/thread_pool.hpp>
#endif


using namespace dsk;


constexpr int fib_sync(int n) noexcept
{
    if(n < 2) return n;
    else      return fib_sync(n - 1) + fib_sync(n - 2);
}

template<class Scheduler>
void test_fib(char const* name, int n = 6)
{
    MESSAGE(name);

    Scheduler sh(start_now);

    auto r = sync_wait([&](this auto&& self, int n) -> task<int>
    {
        if(n < 2)
            DSK_RETURN(n);

        auto[x, y] = DSK_TRY until_all_done(
            start_on(sh, self(n - 1)),
            start_on(sh, self(n - 2)));

        CHECK(! has_err(x));
        CHECK(! has_err(y));
        
        DSK_RETURN(get_val(x) + get_val(y));
    }(n));

    CHECK(! has_err(r));
    CHECK(get_val(r) == fib_sync(n));
}

#define _FIB_TEST(sh) test_fib<sh>(#sh)


TEST_CASE("basic")
{
    SUBCASE("scheduler_fib")
    {
        _FIB_TEST(tbb_implicit_thread_pool);
        _FIB_TEST(tbb_thread_pool);
        _FIB_TEST(asio_thread_pool);
        _FIB_TEST(asio_io_thread_pool);
        _FIB_TEST(simple_thread_pool);
        _FIB_TEST(naive_thread_pool);
    #ifdef BOOST_WINDOWS
        _FIB_TEST(win_thread_pool);
    #endif
    } // SUBCASE("scheduler_fib")

    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }

    SUBCASE("resumer")
    {
        CHECK(any_resumer() == inline_resumer());
        CHECK(any_resumer() != default_io_scheduler_resumer());
        CHECK(any_resumer(DSK_DEFAULT_IO_SCHEDULER) == default_io_scheduler_resumer());

        simple_thread_pool sch1(1, start_now);
        simple_thread_pool sch2(1, start_now);

        auto r = sync_wait
        (
            [&]() -> task<>
            {
                auto thread0 = this_thread::get_id();
                CHECK(DSK_WAIT get_resumer() == inline_resumer());

                DSK_TRY resume_on(sch1);
                auto thread1 = this_thread::get_id();
                CHECK(thread1 != thread0);

                DSK_TRY resume_on(sch2);
                auto thread2 = this_thread::get_id();
                CHECK(thread2 != thread0);
                CHECK(thread2 != thread1);

                DSK_TRY resume_on(sch1, [&]()->task<>
                {
                    CHECK(this_thread::get_id() == thread2);
                    DSK_RETURN();
                }());
                CHECK(this_thread::get_id() == thread1);

                DSK_TRY start_on(sch2, [&]()->task<>
                {
                    CHECK(DSK_WAIT get_resumer() == inline_resumer());
                    CHECK(this_thread::get_id() == thread2);
                    DSK_RETURN();
                }());
                CHECK(this_thread::get_id() == thread2);

                DSK_TRY run_on(sch1, [&]()->task<>
                {
                    CHECK(DSK_WAIT get_resumer() == get_resumer(sch1));
                    CHECK(this_thread::get_id() == thread1);
                    DSK_RETURN();
                }());
                CHECK(this_thread::get_id() == thread1);

                DSK_WAIT set_resumer(sch1);
                CHECK(DSK_WAIT get_resumer() == get_resumer(sch1));

                DSK_TRY solely_run_on(sch2, [&]()->task<>
                {
                    CHECK(DSK_WAIT get_resumer() == get_resumer(sch2));
                    CHECK(this_thread::get_id() == thread2);
                    DSK_RETURN();
                }());
                CHECK(this_thread::get_id() == thread1);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));

    } // SUBCASE("resumer")


    SUBCASE("res_pool")
    {
        auto creator = [i = 0](auto emplace) mutable { emplace(++i); };
        res_pool<int, decltype(creator)> pool(2, creator);

        auto r = sync_wait(until_all_done
        (
            [&]() -> task<>
            {
                auto v1 = DSK_TRY_SYNC pool.try_acquire();
                auto v2 = DSK_TRY_SYNC pool.try_acquire();
                auto r3 = pool.try_acquire();
                CHECK(v1.get() == 1);
                CHECK(v2.get() == 2);
                CHECK(is_err(r3, errc::resource_unavailable));

                DSK_TRY wait_for(std::chrono::milliseconds(1000));
                v2.recycle();

                DSK_RETURN();
            }(),
            [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::milliseconds(500));
                
                auto v2 = DSK_TRY pool.acquire();
                CHECK(*v2 == 2);

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    } // SUBCASE("res_pool")


    SUBCASE("res_pool_map")
    {
        atomic<int> i;
        auto creator = [&i](auto emplace) mutable { emplace(++i); };
        res_pool_map<int, int, decltype(creator)> poolMap(1, creator);

        auto r = sync_wait(until_all_done
        (
            [&]() -> task<>
            {
                auto v1 = DSK_TRY_SYNC poolMap.try_acquire(1);
                auto v2 = DSK_TRY_SYNC poolMap.try_acquire(2);
                auto r3 = poolMap.try_acquire(2);
                CHECK(v1.get() == 1);
                CHECK(v2.get() == 2);
                CHECK(is_err(r3, errc::resource_unavailable));

                DSK_TRY wait_for(std::chrono::milliseconds(1000));
                v2.recycle();

                DSK_RETURN();
            }(),
            [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::milliseconds(500));
                
                auto v2 = DSK_TRY poolMap.acquire(2);
                CHECK(*v2 == 2);

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    } // SUBCASE("res_pool_map")


    SUBCASE("res_queue")
    {
        res_queue<int> queue(2);

        auto r = sync_wait(until_all_done
        (
            run_on(DSK_DEFAULT_IO_SCHEDULER, [&]() -> task<>
            {
                DSK_TRY_SYNC queue.try_enqueue(1);
                DSK_TRY_SYNC queue.try_enqueue(2);
                auto r = queue.try_enqueue(3);
                CHECK(is_err(r, errc::out_of_capacity));

                DSK_TRY wait_for(std::chrono::milliseconds(1000));
                DSK_TRY queue.enqueue(3);
                DSK_TRY queue.enqueue(4);
                DSK_TRY queue.enqueue(5);
                DSK_TRY queue.enqueue(6);

                DSK_TRY wait_for(std::chrono::milliseconds(1000));
                bool isFirst = queue.mark_end();
                CHECK(isFirst);

                auto r2 = DSK_WAIT queue.enqueue(66);
                CHECK(is_err(r2, errc::end_reached));

                DSK_RETURN();
            }()),
            run_on(DSK_DEFAULT_IO_SCHEDULER, [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::milliseconds(500));
                
                int v1 = DSK_TRY queue.dequeue();
                int v2 = DSK_TRY queue.dequeue();
                CHECK(v1 == 1);
                CHECK(v2 == 2);

                auto r = queue.try_dequeue();
                CHECK(is_err(r, errc::resource_unavailable));

                int v3 = DSK_TRY queue.dequeue();
                int v4 = DSK_TRY queue.dequeue();
                CHECK(v3 == 3);
                CHECK(v4 == 4);

                DSK_TRY wait_for(std::chrono::milliseconds(500));
                int v5 = DSK_TRY queue.dequeue();
                int v6 = DSK_TRY queue.dequeue();
                CHECK(v5 == 5);
                CHECK(v6 == 6);

                auto r2 = DSK_WAIT queue.dequeue();
                CHECK(is_err(r2, errc::end_reached));

                DSK_RETURN();
            }())
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    } // SUBCASE("res_queue")


    SUBCASE("cleanup_scopes")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                int total = 0;

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    {
                        DSK_CLEANUP_SCOPE_BEGIN();

                        {
                            DSK_CLEANUP_SCOPE_BEGIN();

                            auto v = default_allocate_unique<int>(6);
                            DSK_CLEANUP_SCOPE_ADD([](auto& total, auto& v) -> task<> { total += *v; DSK_RETURN(); }(total, v));

                            DSK_CLEANUP_SCOPE_END();
                        }

                        CHECK(total == 6);

                        auto v = default_allocate_unique<int>(20);
                        DSK_CLEANUP_SCOPE_ADD(sync_async_op([&](){ total += *v; }));

                        DSK_CLEANUP_SCOPE_END();
                    }

                    CHECK(total == 26);

                    auto v = default_allocate_unique<int>(100);
                    DSK_CLEANUP_SCOPE_ADD(sync_async_op([&](){ total += *v; }));

                    DSK_CLEANUP_SCOPE_END();
                }
                
                CHECK(total == 126);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));

    } // SUBCASE("cleanup_scopes")


    SUBCASE("generator")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                auto gen = []() -> generator<int>
                {
                    DSK_YIELD 1;
                    DSK_YIELD 2;
                    DSK_TRY wait_for(std::chrono::milliseconds(100));
                    DSK_YIELD 6;
                    DSK_RETURN();
                }();

                CHECK(DSK_TRY gen.next() == optional(1));
                CHECK(DSK_TRY gen.next() == optional(2));
                CHECK(DSK_TRY gen.next() == optional(6));
                CHECK(DSK_TRY gen.next() == nullopt);

                auto cleaned = default_allocate_unique<bool>(false);

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    auto scopedGen = [](auto& cleaned) -> generator<int>
                    {
                        auto invoked = default_allocate_unique<bool>(false);

                        DSK_WAIT add_cleanup(sync_async_op([&]()
                        {
                            *invoked = true;
                            *cleaned = true;
                        }));

                        DSK_YIELD 1;
                        DSK_YIELD 2;
                        DSK_RETURN();
                    }(cleaned);

                    CHECK(DSK_TRY scopedGen.next() == optional(1));

                    DSK_CLEANUP_SCOPE_END();
                }

                CHECK(*cleaned);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));

    } // SUBCASE("generator")

} // TEST_CASE("basic")
