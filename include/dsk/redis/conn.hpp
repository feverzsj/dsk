#pragma once

#include <dsk/task.hpp>
#include <dsk/asio/use_async_op.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <dsk/redis/req.hpp>
#include <dsk/redis/adapters.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/detached.hpp>
#include <boost/redis/connection.hpp>
#include <dsk/redis/src.hpp>
#include <memory>


namespace dsk{


using redis_logger    = redis::logger;
using redis_config    = redis::config;
using redis_address   = redis::address;
using redis_ignore_t  = redis::ignore_t;
using redis_operation = redis::operation;
using redis_usage     = redis::usage;

template<class... Ts>
using redis_response          = redis::response<Ts...>;
using redis_generic_response  = redis::generic_response;


// NOTE: the whole connection runs inside a strand, so avoid time-consuming things when resumed,
//       or set a non-inline resumer.
class redis_conn
{
    using conn_type = redis::basic_connection<async_op_io_ctx_strand>;
    std::shared_ptr<conn_type> _conn;

public:
    explicit redis_conn(auto&& ioc,
                        asio::ssl::context ctx = asio::ssl::context{asio::ssl::context::tlsv12_client},
                        std::size_t maxReadSize = std::numeric_limits<std::size_t>::max())
        : _conn(std::make_shared<conn_type>(
            asio::make_strand(DSK_FORWARD(ioc)), mut_move(ctx), maxReadSize))
    {}

    explicit redis_conn(asio::ssl::context ctx = asio::ssl::context{asio::ssl::context::tlsv12_client},
                        std::size_t maxReadSize = std::numeric_limits<std::size_t>::max())
        : redis_conn(
            DSK_DEFAULT_IO_CONTEXT, mut_move(ctx), maxReadSize)
    {}

    ~redis_conn()
    {
        stop();
    }

    template<class Logger = redis_logger>
    void start(redis_config const& cfg = {}, Logger l = Logger{})
    {
        _conn->async_run(cfg, mut_move(l), asio::consign(asio::detached, _conn));
    }

    bool started() const noexcept
    {
        return _conn.operator bool();
    }

    void stop(redis_operation op = redis_operation::all)
    {
        if(_conn)
        {
            _conn->cancel(op);
        }
    }

    auto receive()
    {
        return _conn->async_receive();
    }

    auto exec_get(auto&& res, auto&&... reqs)
    {
        static_assert(sizeof...(reqs));

        auto&& req = to_redis_req(DSK_FORWARD(reqs)...);

        if constexpr(_lval_ref_<decltype(res)> && _lval_ref_<decltype(req)>)
        {
            return _conn->async_exec(req, res);
        }
        else
        {
            return [](conn_type* conn, DSK_LREF_OR_VAL_T(res) res, DSK_LREF_OR_VAL_T(req) req) -> task<>
            {
                DSK_TRY conn->async_exec(req, res);
                DSK_RETURN();
            }
            (_conn.get(), DSK_FORWARD(res), DSK_FORWARD(req));
        }
    }

    template<class R = void>
    auto exec(auto&&... reqs)
    {
        static_assert(sizeof...(reqs));

        if constexpr(_void_<R>)
        {
            return exec_get(redis::ignore, DSK_FORWARD(reqs)...);
        }
        else
        {
            auto&& req = to_redis_req(DSK_FORWARD(reqs)...);

            return [](conn_type* conn, DSK_LREF_OR_VAL_T(req) req) -> task<R>
            {
                R res;
                DSK_TRY conn->async_exec(req, res);
                DSK_RETURN_MOVED(res);
            }
            (_conn.get(), DSK_FORWARD(req));
        }
    }

    bool run_is_canceled() const noexcept
    {
        return _conn->run_is_canceled();
    }

    bool will_reconnect() const noexcept
    {
        return _conn->will_reconnect();
    }

    auto const& get_ssl_context() const noexcept
    {
        return _conn->get_ssl_context();
    }

    void reset_stream()
    {
        _conn->reset_stream();
    }

    auto& next_layer() noexcept
    {
        return _conn->next_layer();
    }

    auto const& next_layer() const noexcept
    {
        return _conn->next_layer();
    }

    void set_receive_response(auto& res)
    {
        _conn->set_receive_response(res);
    }

    redis_usage get_usage() const noexcept
    {
        return _conn->get_usage();
    }
};


} // namespace dsk
