#pragma once

#include <dsk/util/buf.hpp>
#include <dsk/util/handle.hpp>
#include <dsk/expected.hpp>
#include <dsk/curl/str.hpp>
#include <dsk/curl/opt.hpp>
#include <dsk/curl/err.hpp>
#include <dsk/curl/easy.hpp>


namespace dsk{


class curl_multi : public unique_handle<CURLM*, curl_multi_cleanup>
{
    using base = unique_handle<CURLM*, curl_multi_cleanup>;

public:
    using base::base;

    curl_multi()
        : base(curl_multi_init())
    {
        if(! valid())
        {
            throw std::bad_alloc();
        }
    }

    CURLMcode setopt(CURLMoption k, auto v)
    {
        return curl_multi_setopt(checked_handle(), k, v);
    }

    auto setopt(auto&& opt)
    {
        return opt(*this);
    }

    auto setopts(auto&&... opts)
    {
        return foreach_elms_until_err<CURLMcode>(fwd_as_tuple(opts...), [this](auto& opt)
        {
            return opt(*this);
        });
    }

    // NOTE: all added curl_easy must exist until being removed or until after destruction of curl_multi
    CURLMcode add   (curl_easy& e) noexcept { return    curl_multi_add_handle(checked_handle(), e.checked_handle()); }
    CURLMcode remove(curl_easy& e) noexcept { return curl_multi_remove_handle(checked_handle(), e.checked_handle()); }

    template<class T>
    using multi_expected = expected<T, CURLMcode>;

    multi_expected<int> wait(_buf_of_<curl_waitfd> auto&& extra_fds, std::chrono::milliseconds const& timeout) noexcept
    {
        int n_fds = 0;

        if(auto e = curl_multi_wait(checked_handle(), buf_data(extra_fds), buf_size<unsigned>(extra_fds),
                                    static_cast<int>(timeout.count()), &n_fds))
        {
            return e;
        }

        return n_fds;
    }

    multi_expected<int> perform() noexcept
    {
        int runningHandles = 0;
        
        if(auto e = curl_multi_perform(checked_handle(), &runningHandles))
        {
            return e;
        }
        
        return runningHandles;
    }

    multi_expected<int> poll(_buf_of_<curl_waitfd> auto&& extra_fds, std::chrono::milliseconds const& timeout) noexcept
    {
        int n_fds = 0;

        if(auto e = curl_multi_poll(checked_handle(),
                                    buf_data(extra_fds),
                                    buf_size<unsigned>(extra_fds),
                                    static_cast<int>(timeout.count()),
                                    &n_fds))
        {
            return e;
        }

        return n_fds;
    }

    CURLMcode wakeup() noexcept
    {
        return curl_multi_wakeup(checked_handle());
    }

    multi_expected<int> fdset(fd_set* read_fd_set, fd_set* write_fd_set, fd_set* exc_fd_set) noexcept
    {
        int max_fd = 0;

        if(auto e = curl_multi_fdset(checked_handle(), read_fd_set, write_fd_set, exc_fd_set, &max_fd))
        {
            return e;
        }

        return max_fd;
    }

    multi_expected<std::chrono::milliseconds> timeout() noexcept
    {
        long ms = 0;

        if(auto e = curl_multi_timeout(checked_handle(), &ms))
        {
            return e;
        }

        return std::chrono::milliseconds(ms);
    }

    multi_expected<int> socket_action(curl_socket_t s, int ev_bitmask) noexcept
    {
        int runningHandles = 0;
        
        if(auto e = curl_multi_socket_action(checked_handle(), s, ev_bitmask, &runningHandles))
        {
            return e;
        }

        return runningHandles;
    }

    CURLMcode assign(curl_socket_t sockfd, void* sockp) noexcept
    {
        return curl_multi_assign(checked_handle(), sockfd, sockp);
    }

    CURLMsg* next_msg(int& left) noexcept
    {
        return curl_multi_info_read(checked_handle(), &left);
    }

    CURLMsg* next_msg() noexcept
    {
        int left = 0;
        return next_msg(left);
    }

    struct done_msg_t
    {
        CURL*     easy = nullptr;
        CURLcode  err;

        done_msg_t() = default;

        explicit done_msg_t(CURLMsg& m)
            : easy(m.easy_handle), err(m.data.result)
        {
            DSK_ASSERT(m.msg == CURLMSG_DONE);
        }

        explicit operator bool() const noexcept { return easy != nullptr; }
    };

    done_msg_t next_done() noexcept
    {
        while(CURLMsg* m = next_msg())
        {
            if(m->msg == CURLMSG_DONE)
            {
                return done_msg_t{*m};
            }
        }

        return {};
    }
};


} // namespace dsk
