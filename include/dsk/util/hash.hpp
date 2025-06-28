#pragma once


#include <dsk/config.hpp>
#include <dsk/util/str.hpp>
#include <functional>


namespace dsk{


// https://github.com/boostorg/container_hash/blob/develop/include/boost/container_hash/detail/hash_mix.hpp

template<class SizeT = size_t>
constexpr size_t hash_mix(size_t x) noexcept
{
    if constexpr(std::is_same_v<size_t, uint64_t>)
    {
        constexpr uint64_t m = 0xe9846af9b1a615d;

        x ^= x >> 32;
        x *= m;
        x ^= x >> 32;
        x *= m;
        x ^= x >> 28;
    }
    else
    {
        static_assert(std::is_same_v<SizeT, uint32_t>);

        constexpr uint32_t m1 = 0x21f0aaad;
        constexpr uint32_t m2 = 0x735a2d97;

        x ^= x >> 16;
        x *= m1;
        x ^= x >> 15;
        x *= m2;
        x ^= x >> 15;
    }

    return x;
}


// https://github.com/boostorg/container_hash/blob/develop/include/boost/container_hash/hash.hpp

constexpr size_t hash_combine(auto hasher, size_t seed, auto const& v) noexcept
{
    return hash_mix(seed + 0x9e3779b9 + hasher(v));
}

constexpr size_t hash_combine(auto hasher, size_t seed, auto const&... v) noexcept requires(sizeof...(v) > 1)
{
    return (... , (seed = hash_combine(hasher, seed, v)));
}



inline constexpr struct gen_hash_cpo
{
    constexpr size_t operator()(auto hasher, auto const& t) const noexcept
    requires(
        requires{ dsk_gen_hash(*this, hasher, t); })
    {
        return dsk_gen_hash(*this, hasher, t);
    }

    constexpr size_t operator()(auto hasher, auto const& t) const noexcept
    requires(
        requires{ hasher(t); })
    {
        return hasher(t);
    }

}gen_hash;


template<class T, class Hasher>
struct hash_for
{
    constexpr size_t operator()(T const& t) const noexcept
    {
        return gen_hash(Hasher(), t);
    }
};

template<_handle_ T, class Hasher>
struct hash_for<T, Hasher>
{
    using is_transparent = void;

    constexpr size_t operator()(_similar_handle_to_<T> auto const& t) const noexcept
    {
        return gen_hash(Hasher(), get_handle(t));
    }
};

template<_str_class_ T, class Hasher>
struct hash_for<T, Hasher>
{
    using is_transparent = void;

    constexpr size_t operator()(_similar_str_<T> auto const& t) const noexcept
    {
        return gen_hash(Hasher(), t);
    }
};


struct std_hasher
{
    template<class T>
    constexpr size_t operator()(T const& t) const noexcept requires(requires{ std::hash<T>()(t); })
    {
        return std::hash<T>()(t);
    }

    constexpr size_t combine(size_t seed, auto const&... v) noexcept
    {
        return hash_combine(*this, seed, v...);
    }
};

template<class T>
using std_hash_for = hash_for<T, std_hasher>;


} // namespace dsk

#if __has_include(<boost/container_hash/hash.hpp>)

    #include <boost/container_hash/hash.hpp>

    namespace dsk
    {

        struct boost_hasher
        {
            template<class T>
            constexpr size_t operator()(T const& t) const noexcept requires(requires{ boost::hash<T>()(t); })
            {
                return boost::hash<T>()(t);
            }

            constexpr size_t combine(size_t seed, auto const&... v) noexcept
            {
                static_assert(sizeof...(v));
                return (..., (seed = boost::hash_combine(seed, v)));
            }
        };

        template<class T>
        using boost_hash_for = hash_for<T, boost_hasher>;

    } // namespace dsk

#endif
