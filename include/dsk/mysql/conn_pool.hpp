#pragma once

#include <dsk/mysql/conn.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/mysql/connection_pool.hpp>
#include <type_traits>


namespace dsk{


class mysql_pooled_conn : public mysql::pooled_connection
{
    using base = mysql::pooled_connection;

public:
    using base::base;

    mysql_pooled_conn() noexcept = default;

    mysql_pooled_conn(base&& b)
        : base(mut_move(b))
    {}

    static_assert(sizeof(mysql::any_connection) == sizeof(mysql_conn));

    mysql_conn      & get()       noexcept { return static_cast<mysql_conn      &>(base::get()); }
    mysql_conn const& get() const noexcept { return static_cast<mysql_conn const&>(base::get()); }

    mysql_conn      * operator->()       noexcept { return &get(); }
    mysql_conn const* operator->() const noexcept { return &get(); }
};


class mysql_conn_pool : public mysql::connection_pool
{
    using base = mysql::connection_pool;
    
    std::future<void> _run;

    static mysql::pool_params&& to_thread_safe(mysql::pool_params& params)
    {
        params.thread_safe = true;
        return mut_move(params);
    }

public:
    mysql_conn_pool(base&& b)
        : base(mut_move(b))
    {}

    explicit mysql_conn_pool(mysql::pool_params params = {})
        : base(DSK_DEFAULT_IO_CONTEXT, to_thread_safe(params))
    {}

    explicit mysql_conn_pool(auto&& ctxOrExc, mysql::pool_params params = {})
        : base(DSK_FORWARD(ctxOrExc), to_thread_safe(params))
    {}

    ~mysql_conn_pool()
    {
        stop_and_join();
    }

    void start()
    {
        DSK_ASSERT(! started());
        _run = base::async_run(asio::use_future);
    }

    bool started() const noexcept
    {
        return _run.valid();
    }

    void stop()
    {
        if(started())
        {
            base::cancel();
        }
    }

    void join()
    {
        if(started())
        {
            _run.wait();
        }
    }

    void stop_and_join()
    {
        if(started())
        {
            base::cancel();
            _run.wait();
        }
    }

    auto get_conn(auto&... diag)
    {
        return base::async_get_connection(diag..., use_async_op_for<mysql_pooled_conn>);
    }
};


} // namespace dsk
