#pragma once

#include <dsk/util/handle.hpp>
#include <dsk/curl/err.hpp>


namespace dsk{


class curl_share : public unique_handle<CURLSH*, curl_share_cleanup>
{
    using base = unique_handle<CURLSH*, curl_share_cleanup>;

public:
    using base::base;

    curl_share()
        : base(curl_share_init())
    {
        if(! valid())
        {
            throw std::bad_alloc();
        }
    }

    CURLSHcode setopt(CURLSHoption k, curl_lock_data v)
    {
        return curl_share_setopt(checked_handle(), k, v);
    }

    auto setopt(auto&& opt)
    {
        return opt(*this);
    }

    auto setopts(auto&&... opts)
    {
        return foreach_elms_until_err<CURLSHcode>(fwd_as_tuple(opts...), [this](auto& opt)
        {
            return opt(*this);
        });
    }
};


} // namespace dsk
