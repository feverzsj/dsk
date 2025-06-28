#pragma once

#include <dsk/task.hpp>
#include <dsk/res_pool.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/mutex.hpp>
#include <dsk/util/unordered.hpp>
#include <dsk/util/stringify.hpp>
#include <dsk/curl/easy.hpp>
#include <dsk/curl/multi.hpp>
#include <dsk/http/parse.hpp>
#include <dsk/asio/config.hpp>
#include <dsk/asio/endpoint.hpp>
#include <dsk/asio/default_io_scheduler.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>


namespace dsk{


class curl_fields : public curlist
{
public:
    curl_fields() = default;

    explicit curl_fields(auto const&... kvs) { append_field(kvs...); };
    explicit curl_fields(_http_fields_ auto const& flds) { append_fields(flds); };

    void append_field(_str_ auto const& k, _stringifible_ auto const& v)
    {
        curlist::append(cat_as_str(k, ": ", v));
    }

    void append_field(beast::http::field k, _stringifible_ auto const& v)
    {
        append_field(beast::http::to_string(k), v);
    }

    void append_fields(auto const& k, auto const& v, auto const&... kvs)
    {
        static_assert(sizeof...(kvs) % 2 == 0);

        append_field(k, v);

        if constexpr(sizeof...(kvs))
        {
            append_fields(kvs...);
        }
    }

    void append_fields(_http_fields_ auto const& flds)
    {
        for(auto&& f : flds)
        {
            append_field(f.name_string(), f.value());
        }
    }
};


template<class B>
struct cr_buf_wrapper
{
    cr_buf_wrapper(auto&&){}
    static constexpr bool is_buf = false;
};

template<_buf_ B>
struct cr_buf_wrapper<B>
{
    static_assert(_borrowed_byte_buf_<B>);

    lref_or_val_t<B> _b;
    DSK_DEF_MEMBER_IF(! _resizable_buf_<B>, size_t) _curSize{0};

    cr_buf_wrapper(auto&& b) : _b(DSK_FORWARD(b)) {}

    static constexpr bool is_buf = true;
    static constexpr bool is_resizable = _resizable_buf_<B>;

    auto* data() noexcept { return buf_data(_b); }

    size_t size() const noexcept
    {
        if constexpr(is_resizable) return buf_size(_b);
        else                       return _curSize;
    }

    void clear() noexcept
    {
        if constexpr(is_resizable) clear_buf(_b);
        else                       _curSize = 0;
    }

    void remove_suffix(size_t n)
    {
        DSK_ASSERT(n <= size());

        if constexpr(is_resizable) resize_buf(_b, buf_size(_b) - n);
        else                       _curSize -= n;
    }

    auto append(char* d, size_t n)
    {
        if constexpr(is_resizable)
        {
            std::memcpy(buy_buf(_b, n), d, n);
        }
        else
        {
            size_t avl = buf_size(_b) - _curSize;
            n = (std::min)(avl, n);
            std::memcpy(buf_data(_b) + _curSize, d, n);
            _curSize += n;
            return n;
        }
    }
};


class curl_client
{
    enum state_e
    {
        state_reseted, // removed
        state_running, // added
        state_paused,  // added
        state_finished // removed, but not reseted, may have error
    };

    static constexpr bool is_added_state    (state_e st) noexcept { return st == state_running || st == state_paused; }
    static constexpr bool is_removed_state  (state_e st) noexcept { return st == state_reseted || st == state_finished; }
    static constexpr bool is_runnning_state (state_e st) noexcept { return st == state_running; }
    static constexpr bool is_paused_state   (state_e st) noexcept { return st == state_paused; }
    static constexpr bool is_finished_state (state_e st) noexcept { return st == state_finished; }
    static constexpr bool is_resumable_state(state_e st) noexcept { return st == state_paused || st == state_finished; }

    struct socket_data
    {
        asio::ip::tcp::socket::rebind_executor<asio::io_context::executor_type>::other skt;
        curl_socket_t cskt = 0;
        int           acts = 0;
        size_t        id   = 0;

        explicit socket_data(asio::io_context& ioc) noexcept
            : skt(ioc)
        {}

        ~socket_data() { release(); }

        // since we write out dtor, move control won't be implicitly-declared
        socket_data(socket_data&&) = default;
        socket_data& operator=(socket_data&&) = default;

        bool hold_socket() const
        {
            return skt.is_open();
        }

        error_code assign(curl_socket_t s) noexcept
        {
            DSK_ASSERT(! hold_socket());

            DSK_E_TRY_FWD(ep, get_local_endpoint<asio::ip::tcp>(s));

            error_code ec;
            skt.assign(ep.protocol(), s, ec);

            if(! ec)
            {
                cskt = s;
            }

            return ec;
        }

        // Also cancel all outstanding actions.
        // Socket open/close are left to curl
        // CURLOPT_OPENSOCKETFUNCTION/CURLOPT_CLOSESOCKETFUNCTION may be used, but they are unstable.
        void release()
        {
            if(hold_socket())
            {
                error_code ec;
                skt.release(ec);
                DSK_ASSERT(! ec);
                cskt = 0;
                acts = 0;
            }
        }
    };

public:
    class curl_request
    {
        friend class curl_client;

        curl_client& _cl;
        curl_easy    _easy;

        //char errBuf[CURL_ERROR_SIZE] = {};

        any_resumer  _resumer; // should *outside* critical section
        continuation _cont;    // should *outside* critical section
        error_code   _ec;

        state_e _state = state_reseted;

        enum pause_type{ pause_type_none, pause_type_hd, pause_type_wd };
        pause_type _pauseType = pause_type_none;
        size_t _writeCbDataUsed = 0;

        // should *outside* critical section
        void resume(auto&... ioc)
        {
            DSK_ASSERT(is_resumable_state(_state));
            DSK_ASSERT(_cont);
            dsk::resume(continuation(mut_move(_cont)), _resumer, ioc...);
            _resumer.set_inline();
        }

        void resume_from_ioc()
        {
            resume(_cl._ioc);
        }

        // should already inside critical section
        void remove_from_multi(auto const& ec)
        {
            DSK_ASSERT(is_added_state(_state));

            if(! _ec && ec)
            {
                _ec = ec;
            }

            auto err = _cl._multi.remove(_easy);
            DSK_ASSERT(! err);

            _state = state_finished;
        }

        template<pause_type PauseType>
        void mark_paused(size_t used)
        {
            static_assert(PauseType != pause_type_none);

            DSK_ASSERT(is_runnning_state(_state));

            _pauseType = PauseType;
            _writeCbDataUsed = used;
            _state = state_paused;

            DSK_ASSERT(! _ec);
            DSK_ASSERT(! _cl._pausedReq);

            _cl._pausedReq = this;
        }

        template<pause_type PauseType>
        size_t release_data_used_on_pause() noexcept
        {
            static_assert(PauseType != pause_type_none);

            DSK_ASSERT(is_runnning_state(_state) || is_paused_state(_state));
            DSK_ASSERT(_pauseType == PauseType || (_pauseType == pause_type_none && _writeCbDataUsed == 0));

            return std::exchange(_writeCbDataUsed, 0);
        }

        template<pause_type PauseType>
        void clear_pause_if_not() noexcept
        {
            static_assert(PauseType != pause_type_none);

            if(_pauseType != PauseType)
            {
                _pauseType = pause_type_none;
                _writeCbDataUsed = 0;
            }
        }

        void reset_all()
        {
            DSK_ASSERT(is_removed_state(_state));

            _pauseType = pause_type_none;
            _writeCbDataUsed = 0;
            _state = state_reseted;

            // opts
            _easy.reset_opts();

            _easy.setopts(
                curlopt::failonerror,
                curlopt::connect_timeout(std::chrono::seconds(6)),
                curlopt::nosignal,
                curlopt::priv_data(this),
                //curlopt::errbuf(errBuf),
                curlopt::follow_location,
                curlopt::suppress_connect_headers,
                curlopt::disbale_write_cb
                //,curlopt::disbale_header_cb
            );

            _cl.apply_default_opts(*this);
        }

        void on_recycle()
        {
            if(is_paused_state(_state))
            {
                lock_guard lg(_cl._mtx);
                DSK_VERIFY(_cl._multi.remove(_easy));
                _state = state_finished;
            }

            reset_all();
        }

        enum read_type
        {
            // NOTE: hd here means bytes passed to CURLOPT_HEADERFUNCTION. For http protocol, it's header and/or trailer.
            //       wd are bytes passed to CURLOPT_WRITEFUNCTION. For http protocol, it's body
            read_type_hd_wd,
            read_type_hd,
            read_type_wd,
            read_type_hd_until_wd,
            read_type_wd_until_hd,
        };

        // curl will pass multiple responses to write_cb/header_cb when following redirects or tunneling.
        // Tunneling headers is suppressed with CURLOPT_SUPPRESS_CONNECT_HEADERS/curlopt::suppress_connect_headers.
        // In other cases, we suppose there should be header only responses before the last response.
        // We'll skip intermediate responses and return only the last response.
        template<read_type RT, class B, class B2>
        struct async_req_op
        {
                               curl_request&          _cr;
                               cr_buf_wrapper<B >     _bw;
            DSK_NO_UNIQUE_ADDR cr_buf_wrapper<B2>     _bw2;
                               optional_stop_callback _scb; // must be last one defined

            static_assert(RT == read_type_hd_wd ||  ! decltype(_bw2)::is_buf);

            template<size_t Bw, pause_type PauseType>
            static size_t resumed_write_to_bw(char* d, size_t size, size_t nmemb, void* wd)
            {
                static_assert(Bw == 1 || Bw == 2);
                static_assert(PauseType != pause_type_none);

                async_req_op* w = static_cast<async_req_op*>(wd);

                //if(w->exam_stop())
                //{
                //    return -1;
                //}

                size_t n = size * nmemb;

                if(n == 0)
                {
                    return 0;
                }

                size_t prevUsed = w->_cr.template release_data_used_on_pause<PauseType>();

                if(prevUsed > n) // something wrong
                {
                    return 0;
                }

                auto& buf = [&]() -> auto&
                {
                    if constexpr(Bw == 1) return w->_bw;
                    else                  return w->_bw2;
                }();

                if constexpr(buf.is_resizable)
                {
                    buf.append(d + prevUsed, n - prevUsed);
                }
                else
                {
                    size_t m = n - prevUsed;

                    size_t copied = buf.append(d + prevUsed, m);

                    DSK_ASSERT(copied <= m);

                    if(copied < m)
                    {
                        w->_cr.template mark_paused<PauseType>(copied);
                        return CURL_WRITEFUNC_PAUSE;
                    }
                }

                return n;
            }

            template<pause_type PauseType>
            static size_t pause_when_called(char* /*d*/, size_t size, size_t nmemb, void* wd)
            {
                static_assert(PauseType != pause_type_none);

                async_req_op* w = static_cast<async_req_op*>(wd);

                //if(w->exam_stop())
                //{
                //    return -1;
                //}

                size_t n = size * nmemb;

                if(n == 0)
                {
                    return n;
                }

                w->_cr.template mark_paused<PauseType>(0);
                return CURL_WRITEFUNC_PAUSE;
            }

            void init_callbacks()
            {
                _bw.clear();

                if constexpr(RT == read_type_hd_wd)
                {
                    if constexpr(decltype(_bw2)::is_buf)
                    {
                        _bw2.clear();
                        _cr._easy.setopts(
                            curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                            curlopt:: write_cb(resumed_write_to_bw<2, pause_type_wd>, this)
                        );
                    }
                    else
                    {
                        _cr._easy.setopts(
                            curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                            curlopt:: write_cb(resumed_write_to_bw<1, pause_type_wd>, this)
                        );
                        // NOTE: don't use curlopt::header_to_write_cb. It would break pause/resume with other read_type.
                    }
                }
                else if constexpr(RT == read_type_hd)
                {
                    _cr.clear_pause_if_not<pause_type_hd>();
                    _cr._easy.setopts(
                        curlopt::header_cb(resumed_write_to_bw<1, pause_type_hd>, this),
                        curlopt::disbale_write_cb
                    );
                }
                else if constexpr(RT == read_type_wd)
                {
                    _cr.clear_pause_if_not<pause_type_wd>();
                    _cr._easy.setopts(
                        curlopt::disbale_header_cb,
                        curlopt::write_cb(resumed_write_to_bw<1, pause_type_wd>, this)
                    );
                }
                else if constexpr(RT == read_type_hd_until_wd)
                {
                    _cr._easy.setopts(
                        curlopt::header_cb(
                            [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                            {
                                // to skip paused wd before first hd, we set the write_cb when header_cb is called.
                                static_cast<async_req_op*>(wd)->_cr.opts(curlopt::write_cb(pause_when_called<pause_type_hd>, wd));
                                return resumed_write_to_bw<1, pause_type_hd>(d, size, nmemb, wd);
                            },
                            this),
                        curlopt::disbale_write_cb
                    );
                }
                else if constexpr(RT == read_type_wd_until_hd)
                {
                    _cr._easy.setopts(
                        curlopt::disbale_header_cb,
                        curlopt::write_cb(
                            [](char* d, size_t size, size_t nmemb, void* wd) -> size_t
                            {
                                // to skip paused hd before first wd, we set the header_cb when write_cb is called.
                                static_cast<async_req_op*>(wd)->_cr.opts(curlopt::header_cb(pause_when_called<pause_type_wd>, wd));
                                return resumed_write_to_bw<1, pause_type_wd>(d, size, nmemb, wd);
                            },
                            this)
                    );
                }
            }

            using is_async_op = void;

            bool initiate(_async_ctx_ auto&& ctx, _continuation_ auto&& cont)
            {
                //if(exam_stop())
                //{
                //    return false;
                //}

                if(set_canceled_if_stop_requested(_cr._ec, ctx))
                {
                    return false;
                }

                state_e const st = _cr._state;

                DSK_ASSERT(is_removed_state(st) || is_paused_state(st));
                //if(! (is_removed_state(st) || is_paused_state(st)))
                //{
                //    _cr->_ec = sys_errc_t::operation_not_permitted;
                //    return false;
                //}

                DSK_ASSERT(_cr._resumer.is_inline());
                DSK_ASSERT(! _cr._cont);
                _cr._resumer = get_resumer(ctx);
                _cr._cont = DSK_FORWARD(cont);

                if(stop_possible(ctx))
                {
                    _scb.emplace(get_stop_token(ctx), [this]()
                    {
                        {
                            lock_guard lg(_cr._cl._mtx);

                            if(! is_added_state(_cr._state))
                            {
                                return;
                            }

                            _cr.remove_from_multi(make_error_code(errc::canceled));
                        }

                        //_scb.reset();
                        _cr.resume();
                    });
                }

                // start or unpause the request
                {
                    curl_client& cl = _cr._cl;

                    lock_guard lg(cl._mtx);

                    init_callbacks();

                    _cr._state = state_running;
                    
                    DSK_ASSERT(! _cr._ec);

                    if(is_removed_state(st))
                    {
                        if(auto err = cl._multi.add(_cr._easy)) // will set 0 timeout to start transfer
                        {
                            _cr._ec = err;
                        }
                    }
                    else
                    {
                        DSK_ASSERT(is_paused_state(st));

                        if(auto err = _cr._easy.pause(CURLPAUSE_CONT)) // unpause all, NOTE: header_cb/write_cb will be called in this if there are remaining data
                        {                                              // will also use multi and optionally set 0 timeout, so lock it.
                            _cr._ec = err;
                        }
                    }

                    if(_cr._ec)
                    {
                        _cr._state = st;
                        return false;
                    }
                }

                return true;
            }

            bool is_failed() const noexcept
            {
                return has_err(_cr._ec);
            }

            auto take_result()
            {
                return gen_expected_if_no(std::exchange(_cr._ec, {}), [&]()
                {
                    if constexpr(decltype(_bw2)::is_buf) return std::pair(_bw.size(), _bw2.size());
                    else                                 return _bw.size();
                });
            }
        };

        template<read_type RT>
        auto do_read_data(_borrowed_byte_buf_ auto&& b, auto&& b2)
        {
            return async_req_op<RT, decltype(b), decltype(b2)>(
                             *this, DSK_FORWARD(b), DSK_FORWARD(b2));
        }

        template<read_type RT>
        auto do_read_data(_borrowed_byte_buf_ auto&& b)
        {
            return do_read_data<RT>(DSK_FORWARD(b), blank_t());
        }

    public:
        explicit curl_request(curl_client& cl) : _cl(cl) {}

    #ifndef NDEBUG
        ~curl_request()
        {
            DSK_ASSERT(_state == state_reseted || ! is_runnning_state(_state));
        }

        // since we write out dtor, move control won't be implicitly-declared
        curl_request(curl_request&&) = default;
        //curl_request& operator=(curl_request&&) = default;
    #endif

        // still need to check result error
        bool is_finished() noexcept { return is_finished_state(_state); }

        // curlinfo::
    
        auto info(auto const& t) { return _easy.info(t); }

        auto last_response_code() { return _easy.info(curlinfo::response_code); }

        // curlopt::
        // NOTE: some opts are preserved by this class
    
        auto opts(auto&&... t)
        {
            return _easy.setopts(DSK_FORWARD(t)...);
        }

        // hd: data from CURLOPT_HEADERFUNCTION
        // wd: data from CURLOPT_WRITEFUNCTION
        //
        // if buf is lvalue-ref, the ref will be used, otherwise a copy will be used.
        // So, both container and span/view like buf can be used.
        //
        // if buf is not resizable, it will fill the corresponding buf for at most buf_size(buf),
        // and transfer completes when no error is returned && (read size < buf_size(b) || finished())
        // if buf is resizable, it will be filled with all the data left and the buf will be resized to the exact size.
        //
        // NOTE: the data returned is already decoded by curl.
        //       decoding will be applied to original content received with Content-Encoding/Transfer-Encoding header fields.

        // result is expected<size_t>, for actual read size
        auto read_hd         (_borrowed_byte_buf_ auto&& b) { return do_read_data<read_type_hd         >(DSK_FORWARD(b)); }
        auto read_wd         (_borrowed_byte_buf_ auto&& b) { return do_read_data<read_type_wd         >(DSK_FORWARD(b)); }
        auto read_hd_until_wd(_borrowed_byte_buf_ auto&& b) { return do_read_data<read_type_hd_until_wd>(DSK_FORWARD(b)); }
        auto read_wd_until_hd(_borrowed_byte_buf_ auto&& b) { return do_read_data<read_type_wd_until_hd>(DSK_FORWARD(b)); }
        auto read_hd_wd      (_borrowed_byte_buf_ auto&& b) { return do_read_data<read_type_hd_wd      >(DSK_FORWARD(b)); }

        // result is expected<std::pair<size_t, size_t>>, for actual read size
        auto read_hd_wd(_borrowed_byte_buf_ auto&& hd, _borrowed_byte_buf_ auto&& wd)
        {
            return do_read_data<read_type_hd_wd>(DSK_FORWARD(hd), DSK_FORWARD(wd));
        }

        auto read_all(_resizable_byte_buf_ auto& b)
        {
            return read_hd_wd(DSK_FORWARD(b));
        }

        template<_resizable_byte_buf_ B = string>
        task<B> read_all()
        {
            B b;
            DSK_TRY read_hd_wd(b);
            DSK_RETURN_MOVED(b);
        }


        /// http specific methods

        // suppose: all intermediate responses have no body, and only read last response.
        //          all wd belong to last response.
        //          a trailer may follow. After the trailer, the transfer should complete.

        // you can keep on read body, if any.
        auto read_header(_borrowed_byte_buf_ auto&& b)
        {
            return read_hd_until_wd(DSK_FORWARD(b));
        }

        // you can directly read_body() without read_header()
        auto read_body(_borrowed_byte_buf_ auto&& b)
        {
            return read_wd(DSK_FORWARD(b));
        }

        // if there is trailer after read_body().
        auto read_trailer(_borrowed_byte_buf_ auto&& b)
        {
            return read_hd(DSK_FORWARD(b));
        }

        task<> read_header(_http_header_ auto& h)
        {
            string hd;
            DSK_TRY read_hd_until_wd(hd);
            DSK_TRY_SYNC parse_header_trailer(hd, h);
            DSK_RETURN();
        }

        template<class Header = http_response_header>
        task<Header> read_header()
        {
            string hd;
            DSK_TRY read_hd_until_wd(hd);
            Header h;
            DSK_TRY_SYNC parse_header_trailer(hd, h);
            DSK_RETURN_MOVED(h);
        }

        template<_resizable_byte_buf_ B = string>
        task<B> read_body()
        {
            B b;
            DSK_TRY read_wd(b);
            DSK_RETURN_MOVED(b);
        }

        task<> read_trailer(_http_fields_ auto& f)
        {
            string b;
            DSK_TRY read_hd(b);
            DSK_TRY_SYNC parse_fields(b, f);
            DSK_RETURN();
        }

        template<_http_fields_ Fields = http_fields>
        task<Fields> read_trailer()
        {
            string b;
            DSK_TRY read_hd(b);
            Fields f;
            DSK_TRY_SYNC parse_fields(b, f);
            DSK_RETURN_MOVED(f);
        }

        // trailer is allowed(hd after wd).
        task<> read_response(_http_header_ auto& h, _resizable_byte_buf_ auto& b)
        {
            string hd;
            DSK_TRY read_hd_wd(hd, b);
            DSK_TRY_SYNC parse_header_trailer(hd, h);
            DSK_RETURN();
        }

        task<> read_response(_http_response_ auto& r)
        {
            return read_response(r.base(), r.body());
        }

        template<_http_response_ R = http_response>
        task<R> read_response()
        {
            R r;
            string hd;
            DSK_TRY read_hd_wd(hd, r.body());
            DSK_TRY_SYNC parse_header_trailer(hd, r.base());
            DSK_RETURN_MOVED(r);
        }

        task<> read_response(_http_request_ auto& req, _http_response_ auto& res)
        {
            auto flds = DSK_TRY_SYNC set_request(req);
            string hd;
            DSK_TRY read_hd_wd(hd, res.body());
            DSK_TRY_SYNC parse_header_trailer(hd, res.base());
            DSK_RETURN();
        }

        template<_http_response_ R = http_response>
        task<R> read_response(_http_request_ auto& req)
        {
            auto flds = DSK_TRY_SYNC set_request(req);
            R r;
            string hd;
            DSK_TRY read_hd_wd(hd, r.body());
            DSK_TRY_SYNC parse_header_trailer(hd, r.base());
            DSK_RETURN_MOVED(r);
        }

        // NOTE: if req.target() is relative, HOST must be specified.
        expected<curl_fields> set_request(_http_request_ auto& req)
        {
            bool hasBody = false;

            using body_type = typename DSK_NO_REF_T(req)::body_type;

            if constexpr(_byte_buf_<typename body_type::value_type>)
            {
                if(! buf_empty(req.body()))
                {
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::postfields(req.body())));
                    hasBody = true;
                }
            }
            else
            {
                static_assert(_same_as_<body_type, http_empty_body>);
            }

            if(! hasBody)
            {
                DSK_E_TRY_ONLY(_easy.setopt(curlopt::postfields(nullptr)));
            }

            switch(req.method())
            {
                case http_verb::head:
                    DSK_ASSERT(! hasBody);
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::nobody));
                    break;

                case http_verb::connect:
                case http_verb::options:
                case http_verb::trace:
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::customrequest(req.method_string())));
                    [[fallthrough]];
                case http_verb::get :
                    DSK_ASSERT(! hasBody);
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::httpget));
                    break;

                case http_verb::put:
                case http_verb::patch:
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::customrequest(req.method_string())));
                    [[fallthrough]];
                case http_verb::post:
                    DSK_ASSERT(hasBody);
                    break;

                default:
                    DSK_E_TRY_ONLY(_easy.setopt(curlopt::customrequest(req.method_string())));
            }


            curl_fields flds(req);

            // you can prevent curl from adding specified headers by adding field lines like "<name>:"
            // 
            // if body is used, "Content-Type: application/x-www-form-urlencoded",
            // and "Expect: 100-continue"(when body size > 1kb for HTTP 1.1)
            // will be set by curl,
            // "Content-Type" can only be overridden with other value.
            if(hasBody && ! req.count("Expect"))
            {
                flds.append("Expect:"); // disable "Expect: 100-continue" set by curl
            }

            if(! req.count("Accept"))
            {
                flds.append("Accept:");
            }

            if(! req.count("User-Agent"))
            {
                flds.append("User-Agent:");
            }

            DSK_E_TRY_ONLY(_easy.setopt(curlopt::httpheader(flds)));

            if(auto it = req.find("Host"); it != req.end())
            {
                DSK_E_TRY_ONLY(_easy.setopt(curlopt::url(it->value())));
                DSK_E_TRY_ONLY(_easy.setopt(curlopt::request_target(req.target())));
            }
            else
            {
                DSK_E_TRY_ONLY(_easy.setopt(curlopt::url(req.target())));
                DSK_E_TRY_ONLY(_easy.setopt(curlopt::request_target(nullptr)));
            }

            return flds;
        }
    };

private:
    // NOTE: all curl callbacks, anything involving _multi,
    //       state_running _easy should inside critical section
    mutex              _mtx;
    asio::io_context&  _ioc;
    asio::steady_timer::rebind_executor<asio::io_context::executor_type>::other _timer{_ioc};

    curl_multi    _multi;
    curl_request* _pausedReq = nullptr;
    unstable_unordered_map<curl_socket_t, socket_data> _sds;
    size_t _idSeq = 0;

    struct req_creator
    {
        curl_client& cl;
        void operator()(auto emplacer){ emplacer(cl).reset_all(); }
    };                                             //^^^^^^^^^^^: cannot be put in curl_request ctor, as its address is not fixed yet there.

    struct req_recycler
    {
        void operator()(curl_request& r){ r.on_recycle(); }
    };

    res_pool<
        curl_request,
        req_creator, req_recycler> _pool;

    static curl_request& get_curl_request(CURL* e)
    {
        DSK_ASSERT(e);

        curl_easy ce(e);
        auto cr = ce.priv_data<curl_request*>();
        [[maybe_unused]] CURL* er = ce.release();

        DSK_ASSERT(! has_err(cr));
        return *get_val(cr);
    }

    void process_socket_action(curl_socket_t cskt, int acts)
    {
        curl_request* req = [&]()
        {
            lock_guard lg(_mtx);
            return process_socket_action_nolock(cskt, acts).second;
        }();

        if(req)
        {
            req->resume_from_ioc();
        }
    }

    std::pair<int, curl_request*> process_socket_action_nolock(curl_socket_t cskt, int acts)
    {
                                        //vvvvvvvvvvvvv: invokes other curl callbacks
        int stillRunning = get_val(_multi.socket_action(cskt, acts));
        curl_request* req = nullptr;

        if(_pausedReq)
        {
            req = std::exchange(_pausedReq, nullptr);
            DSK_ASSERT(! _multi.next_done());
        }
        else if(auto msg = _multi.next_done())
        {
            req = std::addressof(get_curl_request(msg.easy));
            req->remove_from_multi(msg.err);
        }

        //if(stillRunning <= 0)
        //{
        //    _timer.cancel();
        //}

        return {stillRunning, req};
    }

    // I/O process chain
    template<int Act>
    void process_io(size_t id, error_code const& ec, curl_socket_t cskt)
    {
        static_assert(is_oneof(Act, CURL_POLL_IN, CURL_POLL_OUT));

        //if(ec)
        //{
        //    auto m = ec.message();
        //    auto v = ec.value();
        //    bool stop = true;
        //}

        if(ec == asio::error::operation_aborted)
        {
            return;
        }

        curl_request* req = [&]()
        {
            lock_guard lg(_mtx);

            auto[stillRunning, req] = process_socket_action_nolock(cskt, ec ? CURL_CSELECT_ERR : Act);

            if(ec || ! stillRunning)
            {
                return req;
            }

            // sd may be removed(and possibly the same file descriptor is
            // immediately reused) indirectly by process_socket_action_nolock()
        
            // check if still the same sd

            auto it = _sds.find(cskt);

            if(it == _sds.end() || it->second.id != id)
            {
                return req;
            }

            // if same, continue same act.
            socket_data& sd = it->second;

            if(sd.acts & Act)
            {
                constexpr auto wt = (Act == CURL_POLL_IN ? asio::socket_base::wait_read
                                                         : asio::socket_base::wait_write);

                sd.skt.async_wait(wt, [&, id = sd.id, cskt](auto&& ec)
                {
                    process_io<Act>(id, ec, cskt);
                });
            }

            return req;
        }();

        if(req)
        {
            req->resume_from_ioc();
        }
    }

    // should already inside critical section when get called
    static int multi_sock_cb(CURL* e, curl_socket_t cskt, int acts, void* cbp, void* /*sockp*/)
    {
        DSK_ASSERT(cbp);
        curl_client&  cl = *static_cast<curl_client*>(cbp);
        curl_request& cr = get_curl_request(e);

        switch(acts)
        {
            case CURL_POLL_IN:
            case CURL_POLL_OUT:
            case CURL_POLL_INOUT:
                {
                    auto it = cl._sds.find(cskt);

                    if(it == cl._sds.end())
                    {
                        socket_data sd(cl._ioc);

                        if(auto ec = sd.assign(cskt))
                        {
                            cr.remove_from_multi(ec);
                            return 0; // don't return -1 for error, otherwise all transfers in multi will be aborted.
                        }

                        sd.id = ++ cl._idSeq;
                        it = cl._sds.try_emplace(cskt, mut_move(sd)).first;
                    }

                    socket_data& sd = it->second;

                    if((acts & CURL_POLL_IN) && !(sd.acts & CURL_POLL_IN))
                    {
                        sd.skt.async_wait(asio::socket_base::wait_read, [&, id = sd.id, cskt](auto&& ec)
                        {
                            cl.process_io<CURL_POLL_IN>(id, ec, cskt);
                        });
                    }

                    if((acts & CURL_POLL_OUT) && !(sd.acts & CURL_POLL_OUT))
                    {
                        sd.skt.async_wait(asio::socket_base::wait_write, [&, id = sd.id, cskt](auto&& ec)
                        {
                            cl.process_io<CURL_POLL_OUT>(id, ec, cskt);
                        });
                    }

                    sd.acts = acts;
                }
                break;
            case CURL_POLL_REMOVE:
                {
                    DSK_VERIFY(cl._sds.erase(cskt));
                }
                break;
            default:
                DSK_ASSERTF(true, "multi_sock_cb: unknown action");
        }

        return 0;
    }

    // should already inside critical section when get called
    static int multi_timer_cb(CURLM*, long ms, void* cbp)
    {
        DSK_ASSERT(cbp);
        curl_client& cl = *static_cast<curl_client*>(cbp);

        if(ms > 0)
        {
            cl._timer.expires_after(std::chrono::milliseconds(ms));
            cl._timer.async_wait([&](auto&& ec)
            {
                if(! ec)
                {
                    cl.process_socket_action(CURL_SOCKET_TIMEOUT, 0);
                }
            });
        }
        else if(ms == 0) // 0 timeout is for kicking off process, it should not be canceled like in timer handle.
        {
            //cl._timer.cancel();

            asio::post(cl._ioc, [&]()
            {
                cl.process_socket_action(CURL_SOCKET_TIMEOUT, 0);
            });
        }
        else // ms < 0
        {
            cl._timer.cancel();
        }

        return CURLM_OK;
    }

    unique_function<void(curl_request&)> _defaultOptsFunc;
    string _defaultProxy;

    // called in curl_request.reset_all()
    void apply_default_opts(curl_request& cr)
    {
        cr.opts(curlopt::proxy(_defaultProxy));

        if(_defaultOptsFunc)
        {
            _defaultOptsFunc(cr);
        }
    }

public:
    explicit curl_client(size_t cap, asio::io_context& ioc = DSK_DEFAULT_IO_CONTEXT)
        : _ioc(ioc), _pool(cap, req_creator(*this), req_recycler())
    {
        _multi.setopts(
            curlmopt::socket_cb( multi_sock_cb, this),
            curlmopt:: timer_cb(multi_timer_cb, this)
        );
    }

    curl_client(curl_client const&) = delete;
    curl_client& operator=(curl_client const&) = delete;
    curl_client(curl_client&&) = delete;
    curl_client& operator=(curl_client&&) = delete;

    auto& io_ctx() noexcept { return _ioc; }

    // do not call clear_unused() when running
    auto& pool() noexcept { return _pool; }

    auto acquire_request()
    {
        return _pool.acquire();
    }

    /// settings

    auto opts(auto&&... t)
    {
        lock_guard lg(_mtx);
        return _multi.setopts(DSK_FORWARD(t)...);
    }

    void default_opts_func(auto&& f)
    {
        lock_guard lg(_mtx);
        _defaultOptsFunc = DSK_FORWARD(f);
    }

    void clear_default_opts_func()
    {
        lock_guard lg(_mtx);
        _defaultOptsFunc = nullptr;
    }

    string const& default_proxy() const noexcept
    {
        return _defaultProxy;
    }

    void default_proxy(_byte_str_ auto const& u)
    {
        lock_guard lg(_mtx);
        assign_str(_defaultProxy, u);
    }

    void clear_default_proxy()
    {
        lock_guard lg(_mtx);
        _defaultProxy.clear();
    }

    /// http specific methods

    task<> read_response(_http_request_ auto& req, _http_response_ auto& res)
    {
        auto cr = DSK_TRY acquire_request();
        DSK_TRY cr->read_response(req, res);
        DSK_RETURN();
    }

    template<_http_response_ R = http_response>
    task<R> read_response(_http_request_ auto& req)
    {
        auto cr = DSK_TRY acquire_request();
        DSK_TRY_RETURN(cr->read_response(req));
    }
};


} // namespace dsk
