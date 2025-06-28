#pragma once

#include <dsk/err.hpp>
#include <grpcpp/support/status.h>


namespace dsk{


// NOTE: for Server, an error only means cq.Next() gives a ok == false.
enum class [[nodiscard]] grpc_errc
{
    server_call_canceled = 1,
    server_call_start_failed,
    server_call_finish_failed,
    server_call_write_failed,
    server_bidi_unary_get_no_req,
    client_bidi_unary_get_no_res,
    send_initial_metadata_failed,
};

class grpc_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "grpc"; }

    std::string message(int condition) const override
    {
        switch(static_cast<grpc_errc>(condition))
        {
            case grpc_errc::server_call_canceled         : return "server call canceled";
            case grpc_errc::server_call_start_failed     : return "server call start failed";
            case grpc_errc::server_call_finish_failed    : return "server call finish failed";
            case grpc_errc::server_call_write_failed     : return "server call write failed";
            case grpc_errc::server_bidi_unary_get_no_req : return "server bidi unary get no request";
            case grpc_errc::client_bidi_unary_get_no_res : return "client bidi unary get no response";
            case grpc_errc::send_initial_metadata_failed : return "send initial metadata failed";
        }

        return "undefined";
    }
};

inline constexpr grpc_err_category g_grpc_err_cat;



inline bool dsk_has_err(has_err_cpo, grpc::Status const& s) noexcept
{
    return ! s.ok();
}

inline auto dsk_get_err(get_err_cpo, grpc::Status const& s) noexcept
{
    return s.error_code();
}


class grpc_status_category : public error_category
{
public:
    char const* name() const noexcept override { return "grpc status"; }

    std::string message(int condition) const override
    {
        switch(static_cast<grpc::StatusCode>(condition))
        {
            case grpc::CANCELLED           : return "CANCELLED";
            case grpc::UNKNOWN             : return "UNKNOWN";
            case grpc::INVALID_ARGUMENT    : return "INVALID_ARGUMENT";
            case grpc::DEADLINE_EXCEEDED   : return "DEADLINE_EXCEEDED";
            case grpc::NOT_FOUND           : return "NOT_FOUND";
            case grpc::ALREADY_EXISTS      : return "ALREADY_EXISTS";
            case grpc::PERMISSION_DENIED   : return "PERMISSION_DENIED";
            case grpc::UNAUTHENTICATED     : return "UNAUTHENTICATED";
            case grpc::RESOURCE_EXHAUSTED  : return "RESOURCE_EXHAUSTED";
            case grpc::FAILED_PRECONDITION : return "FAILED_PRECONDITION";
            case grpc::ABORTED             : return "ABORTED";
            case grpc::OUT_OF_RANGE        : return "OUT_OF_RANGE";
            case grpc::UNIMPLEMENTED       : return "UNIMPLEMENTED";
            case grpc::INTERNAL            : return "INTERNAL";
            case grpc::UNAVAILABLE         : return "UNAVAILABLE";
            case grpc::DATA_LOSS           : return "DATA_LOSS";
            default:;
        }

        return "undefined";
    }
};

inline constexpr grpc_status_category g_grpc_status_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, grpc_errc, g_grpc_err_cat)
DSK_REGISTER_ERROR_CODE_ENUM(grpc, StatusCode, ::dsk::g_grpc_status_cat)
