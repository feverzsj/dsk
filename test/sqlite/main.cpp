#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
#include <doctest/doctest.h>

#include <dsk/task.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/sqlite3/conn.hpp>
#include <dsk/sqlite3/stmt.hpp>
#include <dsk/sqlite3/global.hpp>
#include <dsk/sqlite3/header.hpp>
#include <dsk/sqlite3/settings.hpp>
// #include <dsk/sqlite3/spatialite.hpp>
#include <dsk/sqlite3/tokenizer/guni.hpp>


TEST_CASE("sqlite")
{
    using namespace dsk;

    SUBCASE("flags")
    {
        static_assert(sqlite3_flags_t().value == 0);
        static_assert(sqlite3_flags_t().ro ().value == SQLITE_OPEN_READONLY);
        static_assert(sqlite3_flags_t().rw ().value == SQLITE_OPEN_READWRITE);
        static_assert(sqlite3_flags_t().rwc().value == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
        static_assert(sqlite3_flags_t().mem().value == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY));
        static_assert(sqlite3_flags_t().uri         ().value == SQLITE_OPEN_URI         );
        static_assert(sqlite3_flags_t().fullmutex   ().value == SQLITE_OPEN_FULLMUTEX   );
        static_assert(sqlite3_flags_t().nomutex     ().value == SQLITE_OPEN_NOMUTEX     );
        static_assert(sqlite3_flags_t().sharedcache ().value == SQLITE_OPEN_SHAREDCACHE );
        static_assert(sqlite3_flags_t().privatecache().value == SQLITE_OPEN_PRIVATECACHE);
        static_assert(sqlite3_flags_t().nofollow    ().value == SQLITE_OPEN_NOFOLLOW    );
        static_assert(sqlite3_flags_t().exrescode   ().value == SQLITE_OPEN_EXRESCODE   );
        
        static_assert(sqlite3_flags_t().nomutex().privatecache().uri().exrescode().value
                      == (SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_URI | SQLITE_OPEN_EXRESCODE));

        CHECK(sqlite3_flags_t().rwc().nomutex().privatecache().uri().exrescode().to_uri_query("unix")
              == "mode=rwc&cache=private&vfs=unix");
    } // SUBCASE("flags")

    SUBCASE("conn")
    {
        auto r = sync_wait
        (
            [&]() -> task<>
            {
                sqlite3_conn conn;
                DSK_TRY_SYNC conn.open(":memory:");

                CHECK(conn.file_path() == std::string_view(""));

                CHECK(32767 == DSK_TRY_SYNC conn.exec<int16_t>("SELECT ?", int16_t(32767)));
                CHECK(2147483647 == DSK_TRY_SYNC conn.exec<int32_t>("SELECT ?", 2147483647));
                CHECK(9223372036854775807 == DSK_TRY_SYNC conn.exec<int64_t>("SELECT ?", int64_t(9223372036854775807)));
                CHECK(0.45567f == DSK_TRY_SYNC conn.exec<float>("SELECT ?", 0.45567f));
                CHECK(0.45567 == DSK_TRY_SYNC conn.exec<double>("SELECT ?", 0.45567));
                CHECK(DSK_TRY_SYNC conn.exec<bool>("SELECT ?", true));
                CHECK(! DSK_TRY_SYNC conn.exec<bool>("SELECT ?", false));
                CHECK("hello" == DSK_TRY_SYNC conn.exec<string>("SELECT ?", "hello"));
                CHECK("hello" == DSK_TRY_SYNC conn.exec<string>("SELECT ?", string("hello")));
                CHECK(! DSK_TRY_SYNC conn.exec<optional<tuple<int>>>("SELECT 1 WHERE 1=2"));

                auto stmt = DSK_TRY_SYNC conn.prepare("SELECT value FROM json_each(jsonb_array(1, 2, 3))");
                CHECK(tuple(1) == * DSK_TRY_SYNC stmt.next<optional<tuple<int>>>());
                CHECK(tuple(2) == * DSK_TRY_SYNC stmt.next<optional<tuple<int>>>());
                CHECK(tuple(3) == * DSK_TRY_SYNC stmt.next<optional<tuple<int>>>());
                CHECK(! DSK_TRY_SYNC stmt.next());

                DSK_TRY_SYNC conn.exec_multi(R"(
                    CREATE TABLE company(
                        id VARCHAR(10) NOT NULL PRIMARY KEY,
                        name VARCHAR(100) NOT NULL,
                        tax_id VARCHAR(50) NOT NULL
                    );

                    CREATE TABLE employee(
                        id INTEGER PRIMARY KEY,
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

                auto[first_name, last_name, salary, company_id] = DSK_TRY_SYNC conn.exec<
                    tuple<string, string, int, string>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = ?", 1);
                
                CHECK(first_name == "Efficient");
                CHECK(last_name == "Developer");
                CHECK(salary == 30000);
                CHECK(company_id == "AWC");

                auto emptyRow = DSK_TRY_SYNC conn.exec<
                    optional<tuple<string>>>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE id = ?", 66);

                CHECK(! has_val(emptyRow));

                auto nullCol = DSK_TRY_SYNC conn.exec<
                    optional<string>, 1>(
                    "SELECT first_name, last_name, salary, company_id FROM employee WHERE first_name = ?", "Underpaid");

                CHECK(! has_val(nullCol));

                CHECK(3 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY_SYNC conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Some Inc.', 'QLI', 'QW123456'))");
                    
                    CHECK(4 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                    DSK_CLEANUP_SCOPE_END();
                }

                CHECK(3 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    auto trxn = DSK_TRY conn.make_transaction();
                    
                    DSK_TRY_SYNC conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Some Inc.', 'QLI', 'QW123456'))");
                    
                    DSK_TRY_SYNC trxn.commit();

                    DSK_CLEANUP_SCOPE_END();
                }

                CHECK(4 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    auto sp = DSK_TRY conn.make_savepoint();

                    DSK_TRY_SYNC conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Another Inc.', 'ZLQ', 'ZW223456'))");

                    CHECK(5 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                    DSK_CLEANUP_SCOPE_END();
                }

                CHECK(4 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                {
                    DSK_CLEANUP_SCOPE_BEGIN();

                    auto sp = DSK_TRY conn.make_savepoint();

                    DSK_TRY_SYNC conn.exec(R"(INSERT INTO company (name, id, tax_id) VALUES
                                           ('Another Inc.', 'ZLQ', 'ZW223456'))");


                    DSK_TRY_SYNC sp.release();

                    DSK_CLEANUP_SCOPE_END();
                }

                CHECK(5 == DSK_TRY_SYNC conn.exec<int>("SELECT COUNT(*) FROM company"));

                DSK_RETURN();
            }()
        );

        CHECK(! has_err(r));
    } // SUBCASE("conn")

} // TEST_CASE("sqlite")
