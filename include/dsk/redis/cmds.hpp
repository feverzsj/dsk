#pragma once

#include <dsk/redis/req.hpp>


namespace dsk::redis_cmds{


// https://redis.io/docs/latest/commands/ping/
inline constexpr struct ping_t
{
    void operator()(redis_request& req) const
    {
        req.push("PING");
    }

    auto operator()(_byte_str_ auto&& msg) const noexcept
    {
        return redis_cmd("PING", as_str_of<char>(DSK_FORWARD(msg)));
    }
} ping;


/// Pub/Sub

// https://redis.io/docs/latest/commands/subscribe/
auto subscribe(_byte_str_ auto&&... chs)
{
    return redis_cmd_range<1>("SUBSCRIBE", as_str_of<char>(DSK_FORWARD(chs))...);
}

// https://redis.io/docs/latest/commands/unsubscribe/
auto unsubscribe(_byte_str_ auto&&... chs)
{
    if constexpr(sizeof...(chs))
    {
        return redis_cmd_range<1>("UNSUBSCRIBE", as_str_of<char>(DSK_FORWARD(chs))...);
    }
    else
    {
        return redis_cmd("UNSUBSCRIBE");
    }
}

// https://redis.io/docs/latest/commands/publish/
auto publish(_byte_str_ auto&& ch, auto&& msg)
{
    return redis_cmd("PUBLISH", as_str_of<char>(DSK_FORWARD(ch)), DSK_FORWARD(msg));
}


/// String

// https://redis.io/docs/latest/commands/set/
auto set(_byte_str_ auto&& k, _byte_str_ auto&& v, auto&&... opts)
{
    return redis_cmd("SET", as_str_of<char>(DSK_FORWARD(k)), as_str_of<char>(DSK_FORWARD(v)), DSK_FORWARD(opts)...);
}

// https://redis.io/docs/latest/commands/get/
auto get(_byte_str_ auto&& k)
{
    return redis_cmd("GET", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/mget/
auto mget(_byte_str_ auto&&... ks)
{
    return redis_cmd_range<1>("MGET", as_str_of<char>(DSK_FORWARD(ks))...);
}

// https://redis.io/docs/latest/commands/incr/
auto incr(_byte_str_ auto&& k)
{
    return redis_cmd("INCR", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/append/
auto append(_byte_str_ auto&& k, _byte_str_ auto&& v)
{
    return redis_cmd("APPEND", as_str_of<char>(DSK_FORWARD(k)), as_str_of<char>(DSK_FORWARD(v)));
}


/// Generic

// https://redis.io/docs/latest/commands/keys/
auto keys(_byte_str_ auto&& pattern)
{
    return redis_cmd("KEYS", as_str_of<char>(DSK_FORWARD(pattern)));
}

// https://redis.io/docs/latest/commands/exists/
auto exists(_byte_str_ auto&&... ks)
{
    return redis_cmd_range<1>("EXISTS", as_str_of<char>(DSK_FORWARD(ks))...);
}

// https://redis.io/docs/latest/commands/expire/
auto expire(_byte_str_ auto&& k, std::chrono::seconds const& d, auto&&... opts)
{
    return redis_cmd("EXPIRE", as_str_of<char>(DSK_FORWARD(k)), d.count(), DSK_FORWARD(opts)...);
}

// https://redis.io/docs/latest/commands/ttl/
auto ttl(_byte_str_ auto&& k)
{
    return redis_cmd("TTL", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/persist/
auto persist(_byte_str_ auto&& k)
{
    return redis_cmd("PERSIST", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/scan/
auto scan(auto&& cursor, auto&&... opts)
{
    return redis_cmd("SCAN", DSK_FORWARD(cursor), DSK_FORWARD(opts)...);
}

// https://redis.io/docs/latest/commands/del/
auto del(_byte_str_ auto&&... ks)
{
    return redis_cmd_range<1>("DEL", as_str_of<char>(DSK_FORWARD(ks))...);
}

// https://redis.io/docs/latest/commands/info/
auto info(_byte_str_ auto&&... section)
{
    return redis_cmd_range<1>("INFO", as_str_of<char>(DSK_FORWARD(section))...);
}


/// Hashes

// https://redis.io/docs/latest/commands/hset/
auto hset(_byte_str_ auto&& k, auto&&... fvs)
{
    return redis_cmd_key_range<2>("HSET", DSK_FORWARD(k), DSK_FORWARD(fvs)...);
}

// https://redis.io/docs/latest/commands/hget/
auto hget(_byte_str_ auto&& k, _byte_str_ auto&& f)
{
    return redis_cmd("HGET", as_str_of<char>(DSK_FORWARD(k)), as_str_of<char>(DSK_FORWARD(f)));
}

// https://redis.io/docs/latest/commands/hgetall/
auto hgetall(_byte_str_ auto&& k)
{
    return redis_cmd("HGETALL", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/hmget/
auto hmget(_byte_str_ auto&& k, auto&&... fs)
{
    return redis_cmd_key_range<1>("HMGET", DSK_FORWARD(k), DSK_FORWARD(fs)...);
}


/// Sets

// https://redis.io/docs/latest/commands/sadd/
auto sadd(_byte_str_ auto&& k, auto&&... ms)
{
    return redis_cmd_key_range<1>("SADD", DSK_FORWARD(k), DSK_FORWARD(ms)...);
}

// https://redis.io/docs/latest/commands/smembers/
auto smembers(_byte_str_ auto&& k)
{
    return redis_cmd("SMEMBERS", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/scard/
auto scard(_byte_str_ auto&& k)
{
    return redis_cmd("SCARD", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/sismember/
auto sismember(_byte_str_ auto&& k, auto&& m)
{
    return redis_cmd("SISMEMBER", as_str_of<char>(DSK_FORWARD(k)), DSK_FORWARD(m));
}

// https://redis.io/docs/latest/commands/sdiff/
auto sdiff(_byte_str_ auto&&... ks)
{
    return redis_cmd_range<1>("SDIFF", as_str_of<char>(DSK_FORWARD(ks))...);
}

// https://redis.io/docs/latest/commands/sdiffstore/
auto sdiffstore(_byte_str_ auto&& dest, _byte_str_ auto&&... ks)
{
    return redis_cmd_key_range<1>("SDIFFSTORE", DSK_FORWARD(dest), as_str_of<char>(DSK_FORWARD(ks))...);
}

// https://redis.io/docs/latest/commands/srem/
auto srem(_byte_str_ auto&& k, auto&&... ms)
{
    return redis_cmd_key_range<1>("SREM", DSK_FORWARD(k), DSK_FORWARD(ms)...);
}


/// Sorted sets

// https://redis.io/docs/latest/commands/zadd/
auto zadd(_byte_str_ auto&& k, auto&&... sms)
{
    return redis_cmd_key_range<2>("ZADD", DSK_FORWARD(k), DSK_FORWARD(sms)...);
}

// https://redis.io/docs/latest/commands/zrange/                             //vv: use reference, as redis_cmd() catch by ref.
auto zrange(_byte_str_ auto&& k, std::integral auto&& start, std::integral auto&& stop, auto&&... opts)
{
    return redis_cmd("ZRANGE", as_str_of<char>(DSK_FORWARD(k)), start, stop, DSK_FORWARD(opts)...);
}


/// Lists

// https://redis.io/docs/latest/commands/lpush/
auto lpush(_byte_str_ auto&& k, auto&&... vs)
{
    return redis_cmd_key_range<1>("LPUSH", DSK_FORWARD(k), DSK_FORWARD(vs)...);
}

// https://redis.io/docs/latest/commands/rpush/
auto rpush(_byte_str_ auto&& k, auto&&... vs)
{
    return redis_cmd_key_range<1>("RPUSH", DSK_FORWARD(k), DSK_FORWARD(vs)...);
}

// https://redis.io/docs/latest/commands/lrange/
auto lrange(_byte_str_ auto&& k, std::integral auto&& start, std::integral auto&& stop)
{
    return redis_cmd("LRANGE", as_str_of<char>(DSK_FORWARD(k)), start, stop);
}

// https://redis.io/docs/latest/commands/llen/
auto llen(_byte_str_ auto&& k)
{
    return redis_cmd("LLEN", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/lpop/
auto lpop(_byte_str_ auto&& k, std::integral auto&&... n)
{
    static_assert(sizeof...(n) <= 1);
    return redis_cmd("LPOP", as_str_of<char>(DSK_FORWARD(k)), n...);
}

// https://redis.io/docs/latest/commands/rpop/
auto rpop(_byte_str_ auto&& k, std::integral auto&&... n)
{
    static_assert(sizeof...(n) <= 1);
    return redis_cmd("RPOP", as_str_of<char>(DSK_FORWARD(k)), n...);
}


/// Streams

// https://redis.io/docs/latest/commands/xadd/
auto xadd(_byte_str_ auto&& k, auto&&... fvs)
{
    return redis_cmd_key_range<2>("XADD", DSK_FORWARD(k), DSK_FORWARD(fvs)...);
}

// https://redis.io/docs/latest/commands/xread/
auto xread(auto&&... args)
{
    static_assert(sizeof...(args));
    return redis_cmd("XREAD", DSK_FORWARD(args)...);
}

// https://redis.io/docs/latest/commands/xrange/
auto xrange(_byte_str_ auto&& k, auto&& start, auto&& end, std::integral auto&&... count)
{
    static_assert(sizeof...(count) <= 1);

    if constexpr(sizeof...(count))
    {
        return redis_cmd("XRANGE", as_str_of<char>(DSK_FORWARD(k)), start, end, "COUNT", count...);
    }
    else
    {
        return redis_cmd("XRANGE", as_str_of<char>(DSK_FORWARD(k)), start, end);
    }
}

// https://redis.io/docs/latest/commands/xlen/
auto xlen(_byte_str_ auto&& k)
{
    return redis_cmd("XLEN", as_str_of<char>(DSK_FORWARD(k)));
}

// https://redis.io/docs/latest/commands/xdel/
auto xdel(_byte_str_ auto&& k, auto&&... ids)
{
    return redis_cmd_key_range<1>("XDEL", DSK_FORWARD(k), DSK_FORWARD(ids)...);
}

// https://redis.io/docs/latest/commands/xtrim/
auto xtrim(_byte_str_ auto&& k, auto&&... args)
{
    static_assert(sizeof...(args) >= 2);
    return redis_cmd("XTRIM", as_str_of<char>(DSK_FORWARD(k)), DSK_FORWARD(args)...);
}


// Transactions

// https://redis.io/docs/latest/commands/multi/
inline constexpr auto multi = redis_cmd("MULTI");

// https://redis.io/docs/latest/commands/exec/
inline constexpr auto exec = redis_cmd("EXEC");

// https://redis.io/docs/latest/commands/discard/
inline constexpr auto discard = redis_cmd("DISCARD");

// https://redis.io/docs/latest/commands/watch/
auto watch(_byte_str_ auto&&... ks)
{
    return redis_cmd_range<1>("WATCH", as_str_of<char>(DSK_FORWARD(ks))...);
}


} // namespace dsk::redis_cmds
