#pragma once

#include <dsk/config.hpp>
#include <dsk/util/hash.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/list.hpp>
#include <dsk/util/unordered.hpp>


namespace dsk{


// if define the lambda directly in template, each time use it, a different identity will be initialized.
inline constexpr default_gen_list = []<class V>() -> list<V> {};
inline constexpr default_gen_map  = []<class K, class M>() -> stable_unordered_map<K, M> {};


// general lru cache using a Map into List of items
template<
    class Key, class T,
    class Deleter = null_op_t,
    auto  GenList = default_gen_list,
    auto  GenMap  = default_gen_map // map's reference to element should be stable
>
class lru_cache
{
protected:
    struct item_t
    {
        size_t     cost;
        Key const* k;   // hold by map, its reference to element must be always valid
        T          t;

        item_t(size_t c, Key const* k_, auto&&... args)
            : cost(c), k(k_), t(DSK_FORWARD(args)...)
        {}
    };

    using list_type = decltype(GenList.template operator()<item_t>());
    using list_iter = typename list_type::iterator;
    using  map_type = decltype(GenMap.template operator()<Key, list_iter>());

    list_type _items; // (front)mru-...-...-lru(back)
     map_type _iterMap;

    size_t _cost = 0;
    size_t _cap = 0;

    DSK_NO_UNIQUE_ADDR Deleter _del;

    void touch(list_iter it)
    {
        // move to list begin make it a mru
        _items.splice(_items.begin(), _items, it); // insert before
    }

    void preseve_cap()
    {
        while(_cost > _cap)
        {
            item_t& lru = _items.back();

            _del(lru.t);
            _cost -= lru.cost;

            _iterMap.erase(*lru.k);
            _items.pop_back();
        }
    }

public:
    struct statistic_t
    {
        size_t hit = 0;
        size_t total = 0;

        void reset() { hit = 0; total = 0; }
        double hit_rate() const{ return double(hit) / double(total); }
        double miss_rate() const{ return 1 - hit_rate(); }
    };

    statistic_t statistics;


    explicit lru_cache(size_t cap, Deleter del = Deleter())
        : _cap(cap), _del(del)
    {
        DSK_ASSERT(cap > 0);
    }

    auto& deleter() const noexcept { return _del; }
    auto& deleter()       noexcept { return _del; }
    void set_deleter(auto&& d) { _del = DSK_FORWARD(d); }

    size_t used() const noexcept { return _cost; }
    size_t capacity() const noexcept { return _cap; }
    
    void set_capacity(size_t cap)
    {
        DSK_ASSERT(cap > 0);
        _cap = cap;
        preseve_cap();
    }

    size_t item_count() const { return _items.size(); }

    bool contains(auto const& k) const { return _iterMap.contains(k); }

    void clear()
    {
        if constexpr(! _null_op_<Deleter>)
        {
            for(auto& item : _items)
                _del(item.t);
        }

        _items.clear();
        _iterMap.clear();
        _cost = 0;
    }

    void remove(auto const& k)
    {
        if(auto it = _iterMap.find(k); it != _iterMap.end())
        {
            auto lit = it->second;
            _del(lit->t);

            DSK_ASSERT(_cost >= lit->cost);
            _cost -= lit->cost;

            _items.erase(lit);
            _iterMap.erase(it);
        }
    }

    T take(auto const& k)
    {
        if(auto it = _iterMap.find(k); it != _iterMap.end())
        {
            auto lit = it->second;

            DSK_ASSERT(_cost >= lit->cost);
            _cost -= lit->cost;

            T ret = mut_move(lit->t);

            _items.erase(lit);
            _iterMap.erase(it);

            return ret;
        }

        return T();
    }

    // add T(args...) to cache with itemCost, t will be mru in the cache;
    // after a successful insertion, t will be MRU,
    // if totalCost > cap, LRU item will be removed until totalCost <= cap;
    // return the added t(MRU);
    T& add(size_t itemCost, auto&& k, auto&&... args)
    {
        DSK_ASSERT(itemCost <= _cap);
        if(itemCost > _cap)
            throw "item too large";

        auto [mit, isNew] = _iterMap.try_emplace(DSK_FORWARD(k));
        auto&[key, lit] = *mit;

        if(isNew)
        {
            _items.emplace_front(itemCost, &key, DSK_FORWARD(args)...);
            lit = _items.begin();
        }
        else // key already exists, update item_t
        {
            lit->cost = itemCost;
            lit->t    = T(DSK_FORWARD(args)...);
            touch(lit);
        }

        _cost += itemCost;
        preseve_cap();

        return lit->t;
    }

    T* get(auto const& k)
    {
        ++ statistics.total;

        if(auto it = _iterMap.find(k); it != _iterMap.end())
        {
            auto lit = it->second;

            touch(lit);

            ++ statistics.hit;

            return & (lit->t);
        }

        return nullptr;
    }

    T value(auto const& k)
    {
        if(T* p = get(k))
            return *p;
        return T();
    }
};


} // namespace dsk