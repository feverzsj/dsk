#if 0

#include <dsk/util/tuple.hpp>
#include <dsk/util/num_cast.hpp>
#include <dsk/util/stringify.hpp>
#include <span>

std::span<char, 0> sp;

struct stq
{
    int a;
    int b;
};

stq qw(1);

template<size_t N>
constexpr auto fun(){ return N; }
template<>
constexpr auto fun<0>(){ return 1; }

static_assert(fun<0>() == 1);

constexpr auto foo(auto&&... args)
{
    return [](DSK_LREF_OR_VAL_T(args)... args)
    {
        return sizeof...(args);
    }(DSK_FORWARD(args)...);
}

constexpr int qoo(int& )
{
    return 1;
}


int main()
{
    using namespace dsk;

    constexpr auto q = num_cast<float>(INFINITY);
    auto w = num_cast<bool>(-INFINITY);
    auto e = num_cast<bool>(6.00000000000006);
    auto v = num_cast<float>(true);

    static_assert(! _arithmetic_<std::byte>);
    int8_t b = true;
    int8_t u = false;

    auto z = stringify('q');

    foo(1, b, z);

    static_assert(_tuple_like_<tuple<> const&>);

    int p = 0;
    constexpr auto pv = qoo(p);

    constexpr int qy = 0;
    static_assert(_same_as_<decltype(qy), int const>);

}


#else
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/asio/timer.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/pq/err.hpp>
#include <dsk/pq/conn.hpp>
#include <dsk/pq/result.hpp>
#include <dsk/pq/data_types.hpp>


TEST_CASE("pq")
{
    using namespace dsk;
    
    if(! DSK_DEFAULT_IO_SCHEDULER.started())
    {
        DSK_DEFAULT_IO_SCHEDULER.start();
    }

    SUBCASE("conn")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                pq_conn conn;

                DSK_TRY conn.connect("postgresql://postgres:123qweasd@localhost:5432" /*"/dbname"*/);


                CHECK(32767 == DSK_TRY conn.exec<int16_t>("SELECT $1::smallint", int16_t(32767)));
                CHECK(2147483647 == DSK_TRY conn.exec<int32_t>("SELECT $1", 2147483647));
                CHECK(9223372036854775807 == DSK_TRY conn.exec<int64_t>("SELECT $1", int64_t(9223372036854775807)));
                CHECK(0.45567f == DSK_TRY conn.exec<float>("SELECT $1", 0.45567f));
                CHECK(0.45567 == DSK_TRY conn.exec<double>("SELECT $1", 0.45567));
                CHECK(DSK_TRY conn.exec<bool>("SELECT $1", true));
                CHECK(! DSK_TRY conn.exec<bool>("SELECT $1", false));
                CHECK("hello" == DSK_TRY conn.exec<string>("SELECT $1", "hello"));
                CHECK("hello" == DSK_TRY conn.exec<string>("SELECT $1", string("hello")));
                CHECK('X' == DSK_TRY conn.exec<char>("SELECT $1", 'X'));

                vector<std::byte> bytea{std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
                CHECK(bytea == DSK_TRY conn.exec<decltype(bytea)>("SELECT $1", bytea));

                vector<int32_t> int16Arr{1, 2, 6};
                CHECK(int16Arr == DSK_TRY conn.exec<decltype(int16Arr)>("SELECT $1", int16Arr));

                vector<int32_t> int32Arr{INT16_MAX + 1, INT16_MAX + 2, INT16_MAX + 6};
                CHECK(int32Arr == DSK_TRY conn.exec<decltype(int32Arr)>("SELECT $1", int32Arr));

                vector<int64_t> int64Arr{INT32_MAX + 1, INT32_MAX + 2, INT32_MAX + 6};
                CHECK(int64Arr == DSK_TRY conn.exec<decltype(int64Arr)>("SELECT $1", int64Arr));

                vector<float> fltArr{1.1, 2.2, 6.6};
                CHECK(fltArr == DSK_TRY conn.exec<decltype(fltArr)>("SELECT $1", fltArr));

                vector<double> dblArr{FLT_MAX + 1.1, FLT_MAX + 2.2, FLT_MAX + 6.6};
                CHECK(dblArr == DSK_TRY conn.exec<decltype(dblArr)>("SELECT $1", dblArr));

                vector<string> strArr{"cz", "erqqe", "tdaqe"};
                CHECK(strArr == DSK_TRY conn.exec<decltype(strArr)>("SELECT $1", strArr));

                vector<decltype(bytea)> byteaArr{bytea, bytea, bytea};
                CHECK(byteaArr == DSK_TRY conn.exec<decltype(byteaArr)>("SELECT $1", byteaArr));


                int n = 0;
                for([[maybe_unused]]auto&& row : DSK_TRY conn.exec("SELECT 1 WHERE 1=2"))
                {
                    ++n;
                }
                CHECK(n == 0);

                n = 0;
                for(auto&& row: DSK_TRY conn.exec("SELECT 6"))
                {
                    CHECK(6 == DSK_TRY_SYNC row.cols<int>());
                    ++n;
                }
                CHECK(n == 1);

                n = 0;
                for(auto&& row: DSK_TRY conn.exec("SELECT generate_series(1, 3)"))
                {
                    CHECK(n + 1 == DSK_TRY_SYNC row.col<int>(0));
                    ++n;
                }
                CHECK(n == 3);


                {
                    DSK_TRY_SYNC conn.enter_pipeline_mode();

                    DSK_TRY_SYNC conn.add_exec("DROP DATABASE IF EXISTS dsk_test");
                    DSK_TRY_SYNC conn.add_exec("CREATE DATABASE dsk_test");
                    DSK_TRY conn.pipeline_sync();

                    pq_result dropRes = DSK_TRY conn.next_result();
                    CHECK((dropRes && dropRes.status() == PGRES_COMMAND_OK));
                    CHECK(! DSK_TRY conn.next_result());

                    pq_result createRes = DSK_TRY conn.next_result();
                    CHECK((createRes && createRes.status() == PGRES_COMMAND_OK));
                    CHECK(! DSK_TRY conn.next_result());

                    pq_result syncRes = DSK_TRY conn.next_result();
                    CHECK((syncRes && syncRes.status() == PGRES_PIPELINE_SYNC));
                    CHECK(! DSK_TRY conn.next_result());

                    DSK_TRY_SYNC conn.exit_pipeline_mode();
                }

                {
                    DSK_TRY_SYNC conn.enter_pipeline_mode();

                    DSK_TRY_SYNC conn.add_exec("SELECT qewqeqwe");
                    DSK_TRY_SYNC conn.add_exec("SELECT 6");
                    DSK_TRY conn.pipeline_sync();

                    auto r1 = DSK_WAIT conn.next_result();
                    CHECK(is_err(r1, pq_sql_errc::undefined_column));
                    
                    auto res2 = DSK_TRY conn.next_result();
                    CHECK((res2 && res2.status() == PGRES_PIPELINE_ABORTED));
                    CHECK(! DSK_TRY conn.next_result());

                    pq_result syncRes = DSK_TRY conn.next_result();
                    CHECK((syncRes && syncRes.status() == PGRES_PIPELINE_SYNC));
                    CHECK(! DSK_TRY conn.next_result());

                    DSK_TRY_SYNC conn.exit_pipeline_mode();
                }

                // when no cmd undergoing, xxx_result() just returns nullptr result
                CHECK(! DSK_TRY conn.next_result());
                CHECK(! DSK_TRY conn.next_result());

                DSK_TRY conn.connect("user"    , "postgres",
                                     "password", "123qweasd",
                                     "host"    , "localhost",
                                     "port"    , "5432",
                                     "dbname"  , "dsk_test");
                
                DSK_TRY conn.exec_multi(R"(
                    SET NAMES 'UTF8';

                    CREATE TABLE company(
                        id VARCHAR(10) NOT NULL PRIMARY KEY,
                        name VARCHAR(100) NOT NULL,
                        tax_id VARCHAR(50) NOT NULL
                    );

                    CREATE TABLE employee(
                        id INT PRIMARY KEY GENERATED ALWAYS AS IDENTITY,
                        first_name VARCHAR(100) NOT NULL,
                        last_name VARCHAR(100),
                        salary INT CHECK(salary > 0),
                        company_id VARCHAR(10) NOT NULL,
                        FOREIGN KEY (company_id) REFERENCES company(id)
                    );
    
                    INSERT INTO company (name, id, tax_id) VALUES
                        ('Award Winning Company, Inc.', 'AWC', 'IE1234567V'),
                        ('Sector Global Leader Plc', 'SGL', 'IE1234568V'),
                        ('High Growth Startup, Ltd', 'HGS', 'IE1234569V')
                    ;
                    INSERT INTO employee (first_name, last_name, salary, company_id) VALUES
                        ('Efficient', 'Developer', 30000, 'AWC'),
                        ('Lazy', 'Manager', 80000, 'AWC'),
                        ('Good', 'Team Player', 35000, 'HGS'),
                        ('Enormous', 'Slacker', 45000, 'SGL'),
                        ('Coffee', 'Drinker', 30000, 'HGS'),
                        ('Underpaid', NULL, 15000, 'AWC')
                    ;
                    )");

                DSK_TRY conn.reconnect();

                auto[first_name, last_name, salary, company_id] = DSK_TRY conn.exec<
                    tuple<string, string, int, string>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = $1", 1);
                
                CHECK(first_name == "Efficient");
                CHECK(last_name == "Developer");
                CHECK(salary == 30000);
                CHECK(company_id == "AWC");

                auto emptyRow = DSK_TRY conn.exec<
                    optional<tuple<string>>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = $1", 66);

                CHECK(! has_val(emptyRow));

                auto nullCol = DSK_TRY conn.exec<
                    optional<string>, 1>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE first_name = $1", "Underpaid");

                CHECK(! has_val(nullCol));

                {
                    pq_preapred getFirstName("getFirstName");
                    DSK_TRY conn.prepare_for<string>(getFirstName, "SELECT first_name FROM employee WHERE company_id = $1 ORDER BY id");
                
                    pq_result res = DSK_TRY conn.exec(getFirstName, "SGL");
                    CHECK(res.row_cnt() == 1);
                    CHECK(res.col_cnt() == 1);

                    auto name = DSK_TRY_SYNC res.val<std::string_view>(0, 0);
                    CHECK(name == "Enormous");

                    res = DSK_TRY conn.exec(getFirstName, "HGS");
                    CHECK(res.row_cnt() == 2);
                    CHECK(res.col_cnt() == 1);

                    vector<std::string_view> names;
                    for(auto&& row : res)
                    {
                        names.emplace_back(DSK_TRY_SYNC row.col<std::string_view>(0));
                    }
                    CHECK(names == decltype(names){"Good", "Coffee"});

                    res = DSK_TRY conn.describe_prepared(getFirstName);
                    CHECK(res.param_cnt() == 1);
                    CHECK(res.param_oid(0) == pq_oid_text);
                    CHECK(res.fld_cnt() == 1);
                    CHECK(res.fld_oid(0) == pq_oid_varchar);
                    CHECK(res.fld_name(0) == std::string_view("first_name"));

                    //DSK_TRY conn.close_prepared(getFirstName);
                }

                DSK_TRY conn.start_exec(1, // row mode
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE salary > $1 ORDER BY salary", 35000);
                
                CHECK(DSK_TRY conn.next_row(first_name, last_name, salary, company_id));
                CHECK(first_name == "Enormous");
                CHECK(last_name == "Slacker");
                CHECK(salary == 45000);
                CHECK(company_id == "SGL");
                CHECK(DSK_TRY conn.next_row(first_name, last_name, salary, company_id));
                CHECK(first_name == "Lazy");
                CHECK(last_name == "Manager");
                CHECK(salary == 80000);
                CHECK(company_id == "AWC");
                CHECK(! DSK_TRY conn.next_row(first_name, last_name, salary, company_id));


                auto cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 3);

                DSK_CLEANUP_SCOPE
                (
                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Some Inc.', 'QLI', 'QW123456'))");
                    
                    cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                    CHECK(cnt == 4);
                )

                cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 3);

                DSK_CLEANUP_SCOPE
                (
                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Some Inc.', 'QLI', 'QW123456'))");
                    
                    DSK_TRY trxn.commit();
                )

                cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 4);


                DSK_TRY conn.start_copy_to("company", "csv");
                DSK_TRY conn.send_copy_str(
                    "Inc5,QL5,QW6\n",
                    "Inc6,QL6,QW6\n"
                );
                DSK_TRY conn.send_copy_end();

                cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 6);

                DSK_TRY conn.start_copy_from("(SELECT * FROM company WHERE tax_id='QW6' ORDER BY name)", "csv");

                pq_buf l1 = DSK_TRY conn.get_copy_row();
                CHECK(str_view(l1) == "Inc5,QL5,QW6\n");

                pq_buf l2 = DSK_TRY conn.get_copy_row();
                CHECK(str_view(l2) == "Inc6,QL6,QW6\n");

                CHECK(! DSK_TRY conn.get_copy_row());

                CHECK(! DSK_TRY conn.next_result());

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("conn")


    SUBCASE("notify")
    {
        constexpr int nCall = 26;

        auto r = sync_wait(until_all_done
        (
            // listener
            [&]() -> task<>
            {
                pq_conn conn;
                DSK_TRY conn.connect("postgresql://postgres:123qweasd@localhost:5432");

                DSK_TRY conn.listen("count_channel");
                DSK_WAIT add_cleanup(conn.unlisten("count_channel"));

                pq_notify notify0 = DSK_TRY wait_for(std::chrono::seconds(2), conn.get_notify());
                CHECK(notify0.relname() == std::string_view("count_channel"));
                CHECK(notify0.extra() == std::string_view(""));

                for(int i = 0; i < nCall; ++i)
                {
                    pq_notify notify = DSK_TRY wait_for(std::chrono::milliseconds(500), conn.get_notify());
                    CHECK(i == DSK_TRY_SYNC str_to<int>(notify.extra()));
                }

                DSK_RETURN();
            }(),
            // server
            [&]() -> task<>
            {
                DSK_TRY wait_for(std::chrono::seconds(1));

                pq_conn conn;
                DSK_TRY conn.connect("postgresql://postgres:123qweasd@localhost:5432");

                DSK_TRY conn.notify("count_channel");

                for(int i = 0; i < nCall; ++i)
                {
                    DSK_TRY conn.notify("count_channel", i);
                    DSK_TRY wait_for(std::chrono::milliseconds(100));
                }

                DSK_RETURN();
            }()
        ));

        CHECK(! has_err(r));
        CHECK(! has_err(get_elm<0>(get_val(r))));
        CHECK(! has_err(get_elm<1>(get_val(r))));

    }// SUBCASE("notify")

} // TEST_CASE("pq")
#endif
