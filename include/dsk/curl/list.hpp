#pragma once

#include <dsk/config.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/util/concepts.hpp>
#include <curl/curl.h>
#include <new>
#include <memory>


namespace dsk{


// NOTE: must outlive the easy handle
// NOTE: null until you actually append something.
//       So be careful with handle_opt like curlopt::httpheader.
class curlist : public unique_handle<curl_slist*, curl_slist_free_all>
{
public:
    curlist() = default;

    explicit curlist(auto&&... t)
    {
        append(DSK_FORWARD(t)...);
    }

    void append(_char_str_ auto const& s)
    {
        curl_slist* cur = handle();
        curl_slist* h = curl_slist_append(cur, cstr_data(as_cstr(s)));
     
        if(! h)
        {
            throw std::bad_alloc();
        }

        if(! cur)
        {
            reset(h);
        }
    }

    void append(curlist const& l)
    {
        for(auto const& s : l)
        {
            append(s);
        }
    }

    void append(curlist&& l) noexcept
    {
        if(curl_slist* h = handle())
        {            
            while(h->next)
            {
                h = h->next;
            }

            h->next = l.release();
        }
        else
        {
            *this = mut_move(l);
        }
    }

    void append(auto&&... ts) requires(sizeof...(ts) > 1)
    {
        (... , append(DSK_FORWARD(ts)));
    }

    char const* front() const noexcept
    {
        return checked_handle()->data;
    }

    char const* back() const noexcept
    {
        curl_slist* h = checked_handle();

        while(h->next)
        {
            h = h->next;
        }

        return h->data;
    }

    size_t size() const noexcept
    {
        size_t n = 0;

        for(curl_slist* h = handle(); h; h = h->next)
        {
            ++n;
        }

        return n;
    }

    class const_iterator
    {
        curl_slist* _h = nullptr;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = char const*;
        using difference_type   = ptrdiff_t;
        using pointer           = char const**;
        using reference         = char const*;

        explicit const_iterator(curl_slist* h = nullptr) noexcept : _h(h) {}

        char const* operator*() const noexcept { DSK_ASSERT(_h); return   _h->data ; }

        bool operator==(const_iterator const& r) const noexcept
        {
            return _h == r._h;
        }

        const_iterator& operator++() noexcept
        {
            DSK_ASSERT(_h);
            _h = _h->next;
            return *this;
        }
    };

    const_iterator begin() const noexcept { return const_iterator(handle()); }
    const_iterator end  () const noexcept { return const_iterator(); }
};


} // namespace dsk
