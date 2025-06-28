#include <dsk/sync_wait.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/http/file_handler.hpp>


int main(int argc, char* argv[])
{
    using namespace dsk;

    char const* root = argc > 1 ? argv[1] : "./";

    DSK_DEFAULT_IO_SCHEDULER.start();

    auto r = sync_wait
    (
        [&]() -> task<>
        {
            tcp_acceptor acceptor(tcp_endpoint(tcp_v4(), 2626));
                
            http_file_handler fileHandler(root);

            //fileHandler.set_cache_control(std::chrono::seconds(10), "public");

            auto opGrp = DSK_WAIT make_async_op_group();

            for(;;)
            {
                auto conn = DSK_TRY acceptor.accept<http_conn>();

                opGrp.add_and_initiate([](auto conn, auto& fileHandler) -> task<>
                {
                    for(;;)
                    {
                        http_request req;
                    
                        DSK_TRY conn.read(req);

                        auto r = DSK_WAIT fileHandler.handle_request(conn, req);
                    
                        if(has_err(r))
                        {
                            if(is_oneof(get_err(r), asio::error::connection_reset, asio::error::connection_aborted))
                            {
                                break;
                            }

                            DSK_REPORTF("%s\n", get_err(r).message().c_str());

                            http_response res(http_status::internal_server_error, req.version());
                            res.keep_alive(req.keep_alive());
                            DSK_TRY conn.write(res);
                        }

                        if( ! req.keep_alive())
                        {
                            break;
                        }
                    }

                    DSK_RETURN();
                }(std::move(conn), fileHandler));
            }

            DSK_RETURN();
        }()
    );

    if(has_err(r))
    {
        DSK_REPORTF("Failed with: %s\n", get_err(r).message().c_str());
        return -1;
    }

    return 0;
}