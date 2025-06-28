#pragma once

#include <dsk/config.hpp>
#include <array>
#include <ranges>
#include <utility>
#include <iterator>
#include <algorithm>


namespace dsk{


// TODO: ct_map/set based on ct_vector, which is basically an sorted vector.
// a vector class suitable for constexpr context and non-type template parameter.
// NOTE: when used as non-type template parameter, template argument equivalence
//       is only achieved for same type and same binary representation.
//       For ct_vector, we make sure binary representation is equal if they have same sequence of elements.
template<class T, size_t Cap>
class ct_vector
{
public:
    using value_type             = T;
    using pointer                = T*;
    using const_pointer          = const T*; 
    using reference              = value_type&;
    using const_reference        = value_type const&;
    using size_type              = size_t;
    using difference_type        = std::ptrdiff_t;
    using iterator               = T*;
    using const_iterator         = T const*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr ct_vector()
    {}

    constexpr explicit ct_vector(size_type n)
        : _s(n)
    {}

    constexpr explicit ct_vector(size_type n, value_type const& v)
    { assign(n, v); }

    constexpr explicit ct_vector(std::input_iterator auto first, std::sentinel_for<decltype(first)> auto last)
    { assign(first, last); }

    constexpr explicit ct_vector(std::ranges::input_range auto&& r)
    { assign(DSK_FORWARD(r)); }

    constexpr ct_vector(std::initializer_list<value_type> l)
    { assign(l); }

    constexpr ct_vector(ct_vector const& r)
    { assign(r); }

    constexpr ct_vector(ct_vector&& r)
    { assign(mut_move(r)); }

    constexpr ct_vector& operator=(ct_vector const& r)
    {
        assign(r);
        return *this;
    }

    constexpr ct_vector& operator=(ct_vector&& r)
    {
        assign(mut_move(r));
        return *this;
    }

    template<class U, size_t UCap>
    constexpr void assign(ct_vector<U, UCap> const& u)
    {
        if(u.size() > Cap)
            throw "out of capacity";
        std::ranges::copy(u, _d);
        _unchecked_set_size(u.size());
    }

    template<class U, size_t UCap>
    constexpr void assign(ct_vector<U, UCap>&& u)
    {
        if(u.size() > Cap)
            throw "out of capacity";
        std::ranges::move(u, _d);
        _unchecked_set_size(u.size());
        u.clear();
    }

    constexpr void assign(size_type n, value_type const& v)
    {
        if(n > Cap)
            throw "out of capacity";
        std::ranges::fill_n(_d, n, v);
        _unchecked_set_size(n);
    }

    constexpr void assign(std::ranges::input_range auto&& r)
    {
        const auto n = static_cast<size_type>(std::ranges::size(r));
        if(n > Cap)
            throw "out of capacity";
        std::ranges::copy(r, _d);
        _unchecked_set_size(n);
    }

    constexpr void assign(std::input_iterator auto first, std::sentinel_for<decltype(first)> auto last)
    {
        assign(std::ranges::subrange(first, last));
    }

    constexpr void assign(std::initializer_list<value_type> l)
    {
        assign(std::ranges::subrange(l));
    }

    constexpr iterator               begin  ()       noexcept { return _d; }
    constexpr const_iterator         begin  () const noexcept { return _d; }
    constexpr iterator               end    ()       noexcept { return _d + _s; }
    constexpr const_iterator         end    () const noexcept { return _d + _s; }
    constexpr reverse_iterator       rbegin ()       noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin () const noexcept { return reverse_iterator(end()); }
    constexpr reverse_iterator       rend   ()       noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend   () const noexcept { return reverse_iterator(begin()); }
    constexpr const_iterator         cbegin () const noexcept { return _d; }
    constexpr const_iterator         cend   () const noexcept { return _d + _s; }
    constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cbegin()); }
    constexpr const_reverse_iterator crend  () const noexcept { return const_reverse_iterator(cend()); }

    constexpr bool empty() const noexcept { return _s == 0; }
    constexpr size_type size() const noexcept { return _s; }
    static constexpr size_type max_size() noexcept { return Cap; }
    static constexpr size_type capacity() noexcept { return Cap; }

    constexpr void _unchecked_set_size(size_type n)
    {
        if(n < _s)
            std::ranges::fill(_d + n, _d + _s, value_type{});
        _s = n;
    }

    constexpr void resize(size_type n)
    {
        if(n > Cap)
            throw "out of capacity";
        _unchecked_set_size(n);
    }

    constexpr void resize(size_type n, value_type const& v)
    {
        if(n > Cap)
            throw "out of capacity";
        if(n > _s)
            std::ranges::fill(_d + _s, _d + n, v);
        _unchecked_set_size(n);
    }

    constexpr reference       operator[](size_type i)       { if(i >= _s) throw "out of range"; return _d[i]; }
    constexpr const_reference operator[](size_type i) const { if(i >= _s) throw "out of range"; return _d[i]; }
    constexpr reference       front()       { if(! _s) throw "empty"; return _d[0]; }
    constexpr const_reference front() const { if(! _s) throw "empty"; return _d[0]; }
    constexpr reference       back ()       { if(! _s) throw "empty"; return _d[_s - 1]; }
    constexpr const_reference back () const { if(! _s) throw "empty"; return _d[_s - 1]; }
    constexpr       T* data()       noexcept { return _d; }
    constexpr const T* data() const noexcept { return _d; }

    constexpr reference emplace_back(auto&&... args)
    {
        if(_s == Cap)
            throw "out of capacity";
        return _d[_s++] = value_type{DSK_FORWARD(args)...};
    }

    constexpr void push_back(value_type const& v) { emplace_back(v); }
    constexpr void push_back(value_type&& v) { emplace_back(mut_move(v)); }

    constexpr void pop_back()
    {
        if(_s == 0)
            throw "out of size";
        _d[--_s] = value_type{};
    }

    template<class U, size_t UCap>
    constexpr void swap(ct_vector<U, UCap>& u)
    {
        if(u.size() > Cap)
            throw "out of capacity";

        const auto n = std::max(size(), u.size());

        for(size_type i = 0; i < n; ++i)
            std::swap(_d[i], u._d[i]);

        std::swap(_s, u._s);
    }

    constexpr void clear() noexcept
    {
        _unchecked_set_size(0);
    }

    constexpr iterator erase(const_iterator pos)
    {
        return erase(pos, pos + 1);
    }

    constexpr iterator erase(const_iterator first, const_iterator last)
    {
        if(first > last || first < begin() || last > end())
            throw "invalid range";

        std::ranges::move(const_cast<iterator>(last), end(), const_cast<iterator>(first));
        _unchecked_set_size(_s - (last - first));
        return const_cast<iterator>(first);
    }


    constexpr iterator emplace(const_iterator pos, auto&&... args)
    {
        if(pos < begin() || pos > end())
            throw "invalid pos";
        
        if(_s == Cap)
            throw "out of capacity";
        
        auto mpos = const_cast<iterator>(pos);

        std::move_backward(mpos, end(), mpos + 1);
        *mpos = value_type{DSK_FORWARD(args)...};
        ++_s;

        return mpos;
    }

    constexpr iterator insert(const_iterator pos, value_type const& v)
    {
        return emplace(pos, v);
    }

    constexpr iterator insert(const_iterator pos, value_type&& v)
    {
        return emplace(pos, mut_move(v));
    }

    constexpr iterator insert(const_iterator pos, size_type n, value_type const& v)
    {
        if(pos < begin() || pos > end())
            throw "invalid pos";
        
        if(_s + n > Cap)
            throw "out of capacity";
        
        auto mpos = const_cast<iterator>(pos);

        std::ranges::move_backward(mpos, end(), mpos + n);
        std::ranges::fill_n(mpos, n, v);
        _s += n;
        
        return mpos;
    }

    constexpr iterator insert_range(const_iterator pos, std::ranges::input_range auto&& r)
    {
        if(pos < begin() || pos > end())
            throw "invalid pos";

        const auto n = static_cast<size_type>(std::ranges::size(r));
        
        if(_s + n > Cap)
            throw "out of capacity";
        
        auto mpos = const_cast<iterator>(pos);

        std::move_backward(mpos, end(), mpos + n);
        std::ranges::copy(r, mpos);
        _s += n;

        return mpos;
    }

    constexpr iterator insert(const_iterator pos, std::input_iterator auto first, std::sentinel_for<decltype(first)> auto last)
    {
        return insert_range(pos, std::ranges::subrange(first, last));
    }

    constexpr iterator insert(const_iterator pos, std::initializer_list<value_type> l)
    {
        return insert_range(pos, std::ranges::subrange(l));
    }

    constexpr void append_range(std::ranges::input_range auto&& r)
    {
        insert_range(end(), DSK_FORWARD(r));
    }

    constexpr void append(std::input_iterator auto first, std::sentinel_for<decltype(first)> auto last)
    {
        insert(end(), first, last);
    }

    // NOTE: won't used in template argument equivalence
    template<class U, size_t UCap>
    auto operator<=>(ct_vector<U, UCap> const& r) const noexcept
    {
        return std::lexicographical_compare_three_way(begin(), end(), r.begin(), r.end(),
            [](auto& l, auto& r)
            {
                if constexpr(std::three_way_comparable_with<decltype(l), decltype(r)>)
                {
                    return l <=> r;
                }
                else
                {
                    if(l < r)
                        return std::weak_ordering::less;
                    if(r < l)
                        return std::weak_ordering::greater;
    
                    return std::weak_ordering::equivalent;
                }
            }
        );
    }

    size_type  _s = 0;
    value_type _d[Cap + 1] = {}; // +1 for empty ct_vector<T, 0>
};

template<class T>
ct_vector(empty_range_t<T>) -> ct_vector<T, 0>;

template<class T, size_t N>
ct_vector(T const (&)[N]) -> ct_vector<T, N>;

template<class T, size_t N>
ct_vector(std::array<T, N> const&) -> ct_vector<T, N>;



template<size_t Cap>
using ct_index_vector = ct_vector<size_t, Cap>;

template<size_t N>
constexpr auto make_ct_index_vector() noexcept
{
    ct_index_vector<N> vec;

    for(size_t i = 0; i < N; ++i)
        vec.emplace_back(i);

    return vec;
}

template<size_t N>
constexpr auto make_ct_index_vector(size_t v) noexcept
{
    return ct_index_vector<N>(N, v);
}


template<class T, size_t Cap> void _ct_vector_impl(ct_vector<T, Cap>&);
template<class T> concept _ct_vector_ = requires(std::remove_cvref_t<T>& t){ dsk::_ct_vector_impl(t); };


} // namespace dsk
