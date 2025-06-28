#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/concepts.hpp>
#include <dsk/util/stringify.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <exception>


namespace dsk{


using boost::system::error_code;
using boost::system::error_category;
using boost::system::system_error;
using boost::system::system_category;
namespace sys_errc = boost::system::errc;
using sys_errc_t = boost::system::errc::errc_t;


#define DSK_DEF_MAKE_ERROR_CODE(Enum, catInstance)             \
    constexpr dsk::error_code make_error_code(Enum e) noexcept \
    {                                                          \
        return {static_cast<int>(e), catInstance};             \
    }                                                          \
/**/

// also specialize std::is_error_code_enum to make boost::system::error_code convertible to std::error_code
#define DSK_SPECIALIZE_ERROR_CODE_ENUM(Enum)                                   \
    namespace std                                                              \
    {                                                                          \
        template<> struct is_error_code_enum<Enum> : public std::true_type {}; \
    }                                                                          \
    namespace boost::system                                                    \
    {                                                                          \
        template<> struct is_error_code_enum<Enum> : public std::true_type {}; \
    }                                                                          \
/**/

// must put under global namespace
#define DSK_REGISTER_ERROR_CODE_ENUM(NS, Enum, catInstance)    \
    namespace NS{ DSK_DEF_MAKE_ERROR_CODE(Enum, catInstance) } \
    DSK_SPECIALIZE_ERROR_CODE_ENUM(NS::Enum)                   \
/**/

// must put under global namespace
#define DSK_REGISTER_GLOBAL_ERROR_CODE_ENUM(Enum, catInstance) \
    DSK_DEF_MAKE_ERROR_CODE(Enum, catInstance)                 \
    DSK_SPECIALIZE_ERROR_CODE_ENUM(Enum)                       \
/**/


template<class T>
concept _err_code_ = _no_cvref_one_of_<T, boost::system::error_code, std::error_code>;

// template<class T>
// concept _err_enum_ = boost::system::is_error_code_enum<std::remove_cvref_t<T>>::value
//                      ||        std::is_error_code_enum<std::remove_cvref_t<T>>::value;
template<class T>
concept _err_enum_ = requires(T t){ make_error_code(t); } && _enum_<T>;

template<class T>
concept _err_code_or_enum_ = _err_code_<T> || _err_enum_<T>;


inline constexpr struct has_err_cpo
{
    constexpr bool operator()(auto const& t) const noexcept
    requires(requires{
        dsk_has_err(*this, t); })
    {
        return dsk_has_err(*this, t);
    }
} has_err;

inline constexpr struct get_err_cpo
{
    constexpr decltype(auto) operator()(auto&& t) const noexcept
    requires(requires{
        dsk_get_err(*this, DSK_FORWARD(t)); })
    {
        DSK_ASSERT(has_err(t));
        return dsk_get_err(*this, DSK_FORWARD(t));
    }
} get_err;


template<class T>
concept _errorable_ = requires(T t){ has_err(t); get_err(t); };

template<_errorable_ T>
using err_t = DSK_NO_CVREF_T(get_err(std::declval<T>()));


constexpr bool dsk_has_err(has_err_cpo, _err_code_or_enum_ auto const& e) noexcept
{
    return static_cast<bool>(e);
}

constexpr auto&& dsk_get_err(get_err_cpo, _err_code_or_enum_ auto&& e) noexcept
{
    return DSK_FORWARD(e);
}


inline constexpr struct throw_on_err_cpo
{
    constexpr void operator()(auto&& t) const
    requires(requires{
        dsk_throw_on_err(*this, DSK_FORWARD(t)); })
    {
        dsk_throw_on_err(*this, DSK_FORWARD(t));
    }
} throw_on_err;


void dsk_throw_on_err(throw_on_err_cpo, _errorable_ auto&& t)
{
    if(has_err(t))
    {
        throw system_error(get_err(DSK_FORWARD(t)));
    }
}

inline void dsk_throw_on_err(throw_on_err_cpo, std::exception_ptr const& e)
{
    if(DSK_UNLIKELY(e))
    {
        std::rethrow_exception(e);
    }
}


constexpr decltype(auto) to_err_code(_errorable_ auto&& t) noexcept
{
         if constexpr(_err_code_<err_t<decltype(t)>>) return get_err(DSK_FORWARD(t));
    else if constexpr(_err_enum_<err_t<decltype(t)>>) return make_error_code(get_err(DSK_FORWARD(t)));
    else
        static_assert(false, "unsupported type");
}

constexpr auto get_err_or_default(_errorable_ auto&& t) noexcept
{
    if(has_err(t)) return get_err(DSK_FORWARD(t));
    else           return err_t<decltype(t)>();
}

constexpr std::string get_err_msg(_errorable_ auto&& t, _stringifible_ auto&&... noErrs)
{
    if(has_err(t))
    {
        auto&& ec = to_err_code(DSK_FORWARD(t));
        return cat_as_str<std::string>(ec.category().name(), ": ", ec.message());
    }
    else
    {
        return cat_as_str<std::string>(DSK_FORWARD(noErrs)...);
    }
}

constexpr bool is_err(_errorable_ auto const& t, auto const& err) noexcept
{
    if constexpr(_err_code_or_enum_<decltype(t)>)
    {
        return t == err;
    }
    else
    {
        return has_err(t) && get_err(t) == err;
    }
}

constexpr bool is_oneof_errs(_errorable_ auto const& t, auto const&... errs) noexcept
{
    if constexpr(_err_code_or_enum_<decltype(t)>)
    {
        return (... || (t == errs));
    }
    else
    {
        if constexpr(sizeof...(errs))
        {
            if(has_err(t))
            {
                auto const& e = get_err(t);
                return (... || (e == errs));
            }
        }

        return false;
    }
}


enum class [[nodiscard]] errc : uint8_t
{
    failed = 1,
    parse_failed,
    bad_num_cast,
    type_mismatch,
    size_mismatch,
    out_of_bound,
    out_of_capacity,
    end_reached,
    not_found,
    timeout,
    file_exists,
    canceled,
    allocation_failed,
    one_or_more_ops_failed,
    one_or_more_cleanup_ops_failed,
    resource_unavailable,
    invalid_size,
    invalid_input,
    input_not_fully_consumed,
    unknown_size,
    unknown_exception,
    unsupported_op,
    unsupported_type,
    none_err, // used to signal special state.
};

class generic_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "general"; }

    std::string message(int condition) const override
    {
        switch(static_cast<errc>(condition))
        {
            case errc::failed                         : return "operation failed";
            case errc::parse_failed                   : return "parse failed";
            case errc::bad_num_cast                   : return "bad numeric cast";
            case errc::type_mismatch                  : return "type mismatch";
            case errc::size_mismatch                  : return "size mismatch";
            case errc::out_of_bound                   : return "out of bound";
            case errc::out_of_capacity                : return "out of capacity";
            case errc::end_reached                    : return "end reached";
            case errc::not_found                      : return "not found";
            case errc::timeout                        : return "operation timeout";
            case errc::file_exists                    : return "file exists";
            case errc::canceled                       : return "operation canceled";
            case errc::allocation_failed              : return "allocation failed";
            case errc::one_or_more_ops_failed         : return "one or more ops failed";
            case errc::one_or_more_cleanup_ops_failed : return "one or more cleanup ops failed";
            case errc::resource_unavailable           : return "resource unavailable";
            case errc::invalid_size                   : return "invalid size";
            case errc::invalid_input                  : return "invalid input";
            case errc::input_not_fully_consumed       : return "input is not fully consumed";
            case errc::unknown_size                   : return "unknown size";
            case errc::unknown_exception              : return "unknown exception";
            case errc::unsupported_op                 : return "unsupported operation";
            case errc::unsupported_type               : return "unsupported type";
            case errc::none_err                       : return "none error";
        }

        return "undefined";
    }
};

inline constexpr generic_err_category g_generic_err_cat;


template<auto Indices, class EmptyRet = errc>
constexpr decltype(auto) foreach_elms_until_err(auto&& t, auto&& f)
{
    return foreach_elms_until<Indices, EmptyRet>
    (
        DSK_FORWARD(t),
        DSK_FORWARD(f),
        has_err
    );
}

template<class EmptyRet = errc>
constexpr decltype(auto) foreach_elms_until_err(auto&& t, auto&& f)
{
    return foreach_elms_until_err
     <
        make_index_array<uelm_count<decltype(t)>>(),
        EmptyRet
    >
    (DSK_FORWARD(t), DSK_FORWARD(f));
}


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, errc, g_generic_err_cat)
