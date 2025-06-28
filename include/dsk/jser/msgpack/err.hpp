#pragma once

#include <dsk/err.hpp>


namespace dsk{


enum class [[nodiscard]] msgpack_errc
{
    not_enough_input_to_read = 1,
    tag_mismatch,
    seek_failed,
    unget_failed,
    posi_overflow,
    buf_mismatch,
    invalid_ext_size,
    ext_type_mismatch,
};


class msgpack_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "msgpack"; }

    std::string message(int condition) const override
    {
        switch(static_cast<msgpack_errc>(condition))
        {
            case msgpack_errc::not_enough_input_to_read : return "not enough input to read";
            case msgpack_errc::tag_mismatch             : return "tag mismatch";
            case msgpack_errc::seek_failed              : return "seek failed";
            case msgpack_errc::unget_failed             : return "unget failed";
            case msgpack_errc::posi_overflow            : return "positive integer overflow when converting to signed type";
            case msgpack_errc::buf_mismatch             : return "buffer size mismatch";
            case msgpack_errc::invalid_ext_size         : return "invalid ext size";
            case msgpack_errc::ext_type_mismatch        : return "ext type mismatch";
        }

        return "undefined";
    }
};

inline constexpr msgpack_err_category g_msgpack_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, msgpack_errc, g_msgpack_err_cat)
