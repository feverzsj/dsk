#pragma once

#include <dsk/optional.hpp>
#include <dsk/util/buf.hpp>
#include <dsk/util/str.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/num_cast.hpp>
#include <boost/mysql/field.hpp>
#include <boost/mysql/field_view.hpp>
#include <boost/mysql/row.hpp>
#include <boost/mysql/row_view.hpp>
#include <boost/mysql/rows.hpp>
#include <boost/mysql/rows_view.hpp>


namespace std{
    template<class T, size_t E>
    inline constexpr bool ranges::enable_borrowed_range<boost::span<T, E>> = true;

    template<class Ch>
    inline constexpr bool ranges::enable_borrowed_range<boost::core::basic_string_view<Ch>> = true;

    template<class Ch, class Tx>
    inline constexpr bool ranges::enable_borrowed_range<boost::basic_string_view<Ch, Tx>> = true;
}


namespace dsk{


namespace mysql = boost::mysql;


using mysql_field      = mysql::field;
using mysql_field_view = mysql::field_view;
using mysql_row        = mysql::row;
using mysql_row_view   = mysql::row_view;
using mysql_rows       = mysql::rows;
using mysql_rows_view  = mysql::rows_view;
using mysql_date       = mysql::date;
using mysql_time       = mysql::time;
using mysql_datetime   = mysql::datetime;


template<class T>
concept _mysql_field_ = _no_cvref_one_of_<T, mysql_field, mysql_field_view>;


inline constexpr struct get_mysql_field_cpo
{
    constexpr auto operator()(_mysql_field_ auto&& f, auto&& v) const
    requires(requires{
        {dsk_get_mysql_field(*this, DSK_FORWARD(f), DSK_FORWARD(v))} -> _errorable_; })
    {
        return dsk_get_mysql_field(*this, DSK_FORWARD(f), DSK_FORWARD(v));
    }
} get_mysql_field;


errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f,  _arithmetic_ auto& v)
{
    if(f.is_int64 ()) return cvt_num(f.get_int64 (), v);
    if(f.is_uint64()) return cvt_num(f.get_uint64(), v);
    if(f.is_float ()) return cvt_num(f.get_float (), v);
    if(f.is_double()) return cvt_num(f.get_double(), v);

    return errc::type_mismatch;
}

errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f, _byte_buf_ auto& v)
{
    if(f.is_string())
    {
        assign_str(v, std::move(f.get_string()));
        return {};
    }

    if(f.is_blob())
    {
        assign_buf(v, as_buf_of<buf_val_t<decltype(v)>>(std::move(f.get_blob())));
        return {};
    }

    return errc::type_mismatch;
}

errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f, mysql_date& v)
{
    if(f.is_date())
    {
        v = f.get_date();
        return {};
    }

    return errc::type_mismatch;
}

errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f, mysql_time& v)
{
    if(f.is_time())
    {
        v = f.get_time();
        return {};
    }

    return errc::type_mismatch;
}

errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f, mysql_datetime& v)
{
    if(f.is_datetime())
    {
        v = f.get_datetime();
        return {};
    }

    return errc::type_mismatch;
}

template<class Dur>
errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f,
                         std::chrono::time_point<std::chrono::system_clock, Dur>& v)
{
    if(f.is_datetime()) { v = f.get_datetime().get_time_point(); return {}; }
    if(f.is_date    ()) { v = f.get_date    ().get_time_point(); return {}; }
    if(f.is_time    ()) { v = f.get_time    ().get_time_point(); return {}; }

    return errc::type_mismatch;
}

template<class Dur>
errc dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f,
                         std::chrono::time_point<std::chrono::local_t, Dur>& v)
{
    if(f.is_datetime()) { v = f.get_datetime().get_local_time_point(); return {}; }
    if(f.is_date    ()) { v = f.get_date    ().get_local_time_point(); return {}; }
    if(f.is_time    ()) { v = f.get_time    ().get_local_time_point(); return {}; }

    return errc::type_mismatch;
}

auto dsk_get_mysql_field(get_mysql_field_cpo, _mysql_field_ auto&& f, _optional_ auto& v)
{
    if(f.is_null())
    {
        v.reset();
        return decltype(get_mysql_field(f, assure_val(v)))();
    }

    return get_mysql_field(f, assure_val(v));
}


template<class T>
concept _mysql_row_ = _no_cvref_one_of_<T, mysql_row, mysql_row_view>;


template<size_t StartCol = 0>
auto get_fields(_mysql_row_ auto&& row, auto&&... vs)
{
    return get_fields<StartCol>(DSK_FORWARD(row), fwd_as_tuple(DSK_FORWARD(vs)...));
}

template<size_t StartCol = 0>
auto get_fields(_mysql_row_ auto&& row, _tuple_like_ auto&& vs)
{
    DSK_ASSERT(StartCol + elm_count<decltype(vs)> <= row.size());

    return foreach_elms_until_err(DSK_FORWARD(vs), [&]<size_t I>(auto&& v)
    {
        return get_mysql_field(std::forward_like<decltype(row)>(row[StartCol + I]), DSK_FORWARD(v));
    });
}


} // namespace dsk
