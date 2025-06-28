#pragma once

#include <dsk/err.hpp>


namespace dsk{


enum class [[nodiscard]] jerrc
{
    out_of_bound = 1,
    not_a_string,
    not_a_primitive,
    duplicated_key,
    field_not_found,
    non_nullable_field,
    user_conv_failed,
    user_check_failed,
    user_exam_failed,
    invalid_elm_cnt,
    invalid_sub_elm_cnt,
};


class jerr_category : public error_category
{
public:
    char const* name() const noexcept override { return "json"; }

    std::string message(int condition) const override
    {
        switch(static_cast<jerrc>(condition))
        {
            case jerrc::out_of_bound        : return "out of array bound";
            case jerrc::not_a_string        : return "value is not a string";
            case jerrc::not_a_primitive     : return "value is not a primitive";
            case jerrc::duplicated_key      : return "duplicated key";
            case jerrc::field_not_found     : return "field not found";
            case jerrc::non_nullable_field  : return "non nullable field is null";
            case jerrc::user_conv_failed    : return "user conversion(e.g.: jconv) failed";
            case jerrc::user_check_failed   : return "user check failed";
            case jerrc::user_exam_failed    : return "user exam failed";
            case jerrc::invalid_elm_cnt     : return "invalid element count";
            case jerrc::invalid_sub_elm_cnt : return "invalid sub element count";
        }

        return "undefined";
    }
};

inline constexpr jerr_category g_jerr_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, jerrc, g_jerr_cat)
