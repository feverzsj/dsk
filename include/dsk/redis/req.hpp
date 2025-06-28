#pragma once

#include <dsk/util/str.hpp>
#include <boost/redis/request.hpp>


namespace dsk{


namespace redis = boost::redis;

using redis_request = redis::request;


constexpr auto redis_cmd(_byte_str_ auto const& cmd, auto&&... args) noexcept
{
    return [&](redis_request& req)
    {
        return req.push(as_str_of<char>(cmd), DSK_FORWARD(args)...);
    };
}

template<size_t ElmsPerVal>
constexpr auto redis_cmd_range(_byte_str_ auto const& cmd, auto&& arg, auto&&... args) noexcept
{
    return [&](redis_request& req)
    {
        if constexpr(std::ranges::range<decltype(arg)> && ! _str_<decltype(arg)>)
        {
            static_assert(sizeof...(args) == 0);

            constexpr size_t nElm = elm_count<unchecked_range_value_t<decltype(arg)>>;
            static_assert(nElm && nElm % ElmsPerVal == 0);

            return req.push_range(as_str_of<char>(cmd), DSK_FORWARD(arg));
        }
        else
        {
            static_assert((1 + sizeof...(args)) % ElmsPerVal == 0);
            return req.push(as_str_of<char>(cmd), DSK_FORWARD(arg), DSK_FORWARD(args)...);
        }
    };
}

template<size_t ElmsPerVal>
constexpr auto redis_cmd_key_range(_byte_str_ auto const& cmd, _byte_str_ auto&& k, auto&& arg, auto&&... args) noexcept
{
    return [&](redis_request& req)
    {
        if constexpr(std::ranges::range<decltype(arg)> && ! _str_<decltype(arg)>)
        {
            static_assert(sizeof...(args) == 0);

            constexpr size_t nElm = uelm_count<unchecked_range_value_t<decltype(arg)>>;
            static_assert(nElm && nElm % ElmsPerVal == 0);

            return req.push_range(as_str_of<char>(cmd), as_str_of<char>(k), DSK_FORWARD(arg));
        }
        else
        {
            static_assert((1 + sizeof...(args)) % ElmsPerVal == 0);
            return req.push(as_str_of<char>(cmd), as_str_of<char>(DSK_FORWARD(k)), DSK_FORWARD(arg), DSK_FORWARD(args)...);
        }
    };
}


decltype(auto) to_redis_req(auto&& req, auto&&... reqs)
{
    if constexpr(_derived_from_<decltype(req), redis_request>)
    {
        (..., reqs(req));
        DSK_FORWARD(req);
    }
    else
    {
        redis_request r;
        (req(r), ..., reqs(r));
        return r;
    }
}


} // namespace dsk
