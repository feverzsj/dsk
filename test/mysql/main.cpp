#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/until.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/mysql/conn.hpp>
#include <dsk/mysql/conn_pool.hpp>
#include <dsk/mysql/fields.hpp>


TEST_CASE("mysql")
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
                mysql_conn conn;

                DSK_TRY conn.connect(
                {
                    .server_address = mysql_host_and_port{"localhost", 6266},
                    .username = "root",
                    .password = "123qweasd",
                    .database = "", // can also use `USE db` statement
                    .multi_queries = true
                });

                DSK_WAIT add_cleanup(conn.close());

                DSK_TRY conn.exec(R"(
                    -- Connection system variables
                    SET NAMES utf8;

                    -- Database
                    DROP DATABASE IF EXISTS dsk_test;
                    CREATE DATABASE dsk_test;
                    USE dsk_test;

                    CREATE TABLE company(
                        id CHAR(10) NOT NULL PRIMARY KEY,
                        name VARCHAR(100) NOT NULL,
                        tax_id VARCHAR(50) NOT NULL
                    );

                    CREATE TABLE employee(
                        id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                        first_name VARCHAR(100) NOT NULL,
                        last_name VARCHAR(100),
                        salary INT UNSIGNED,
                        company_id CHAR(10) NOT NULL,
                        FOREIGN KEY (company_id) REFERENCES company(id)
                    );
    
                    INSERT INTO company (name, id, tax_id) VALUES
                        ("Award Winning Company, Inc.", "AWC", "IE1234567V"),
                        ("Sector Global Leader Plc", "SGL", "IE1234568V"),
                        ("High Growth Startup, Ltd", "HGS", "IE1234569V")
                    ;
                    INSERT INTO employee (first_name, last_name, salary, company_id) VALUES
                        ("Efficient", "Developer", 30000, "AWC"),
                        ("Lazy", "Manager", 80000, "AWC"),
                        ("Good", "Team Player", 35000, "HGS"),
                        ("Enormous", "Slacker", 45000, "SGL"),
                        ("Coffee", "Drinker", 30000, "HGS"),
                        ("Underpaid", NULL, 15000, "AWC")
                    ;
                    )");

                auto[first_name, last_name, salary, company_id] = DSK_TRY conn.exec<
                    tuple<string, string, int, string>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = {}", 1);
                
                CHECK(first_name == "Efficient");
                CHECK(last_name == "Developer");
                CHECK(salary == 30000);
                CHECK(company_id == "AWC");

                auto emptyRow = DSK_TRY conn.exec<
                    optional<tuple<string>>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = {}", 66);

                CHECK(! has_val(emptyRow));

                auto emptyCol = DSK_TRY conn.exec<
                    optional<string>, 1>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = {}", 6);

                CHECK(! has_val(emptyCol));


                mysql_results results;
                mysql_stmt stmt = DSK_TRY conn.prepare_statement(
                                    "SELECT first_name FROM employee WHERE company_id = ? ORDER BY id");
                
                DSK_TRY conn.exec_get(results, stmt, "SGL");

                CHECK(results.size() == 1);
                CHECK(results.rows().size() == 1);
                CHECK(results.rows().num_columns() == 1);

                std::string_view name;
                DSK_TRY_SYNC get_fields(results.rows()[0], name);

                CHECK(name == "Enormous");

                mysql_exec_state state;
                DSK_TRY conn.start_exec(state, stmt, "HGS");
                CHECK(! state.complete());
                CHECK(state.should_read_rows());

                mysql_rows_view rows = DSK_TRY conn.read_some_rows(state);
                CHECK(state.complete());
                CHECK(! state.should_read_rows());
                CHECK(rows.size() == 2);
                CHECK(rows.num_columns() == 1);
                DSK_TRY_SYNC get_fields(rows[0], name);
                CHECK(name == "Good");
                DSK_TRY_SYNC get_fields(rows[1], name);
                CHECK(name == "Coffee");

                auto cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 3);

                DSK_CLEANUP_SCOPE
                (
                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ("Some Inc.", "QLI", "QW123456"))");
                    
                    cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                    CHECK(cnt == 4);
                )

                cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 3);

                DSK_CLEANUP_SCOPE
                (
                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ("Some Inc.", "QLI", "QW123456"))");
                    
                    DSK_TRY trxn.commit();
                )

                cnt = DSK_TRY conn.exec<int>("SELECT COUNT(*) FROM company");
                CHECK(cnt == 4);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("conn")


    SUBCASE("conn_pool")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                mysql_conn_pool pool(
                {
                    .server_address = mysql_host_and_port{"localhost", 6266},
                    .username = "root",
                    .password = "123qweasd",
                    .database = "dsk_test"
                });

                pool.start();
                CHECK(pool.started());

                auto conn = DSK_TRY pool.get_conn();

                auto cnt = DSK_TRY conn->exec<int>("SELECT COUNT(*) FROM employee");
                
                CHECK(cnt == 6);

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    }// SUBCASE("conn_pool")

} // TEST_CASE("mysql")
