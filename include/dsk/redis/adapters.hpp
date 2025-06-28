#pragma once

#include <dsk/optional.hpp>
#include <dsk/util/tuple.hpp>
#include <boost/redis/connection.hpp>


namespace dsk{


template<class T>
struct redis_single_adapter
{
    static_assert(! _tuple_like_<T>);
    static_assert(! _ref_wrap_<T>);
    static_assert(! _rval_ref_<T>);
    static_assert(! _const_<std::remove_reference_t<T>>);

    using ncvr_type = std::remove_cvref_t<T>;
    using impl_type = typename boost::redis::adapter::detail::impl_map<
                        DSK_CONDITIONAL_T(_optional_<T>, typename ncvr_type::value_type, ncvr_type)>::type;

    T& _v;
    impl_type _impl;

    explicit redis_single_adapter(T& v)
        : _v(v)
    {
        if constexpr(_optional_<T>) _v.reset();
        else                        _impl.on_value_available(_v);
    }

    template<class String>
    void operator()(size_t, boost::redis::resp3::basic_node<String> const& nd, error_code& ec)
    {
        if(nd.data_type == boost::redis::resp3::type::simple_error)
        {
            ec = boost::redis::error::resp3_simple_error;
            return;
        }

        if(nd.data_type == boost::redis::resp3::type::blob_error)
        {
            ec = boost::redis::error::resp3_blob_error;
            return;
        }

        if constexpr(_optional_<T>)
        {
            if(nd.data_type == boost::redis::resp3::type::null)
            {
                DSK_ASSERT(! _v);
                return;
            }

            if(! _v)
            {
                _impl.on_value_available(_v.emplace());
            }

            _impl(*_v, nd, ec);
        }
        else
        {
            if(nd.data_type == boost::redis::resp3::type::null)
            {
                ec = boost::redis::error::resp3_null;
                return;
            }

            _impl(_v, nd, ec);
        }
    }

    static constexpr size_t get_supported_response_size() noexcept
    {
        return 1;
    }
};


template<class T, bool Nested = false>
struct redis_tuple_adapter
{
    static_assert(_tuple_like_<T>);
    static_assert(! _const_<std::remove_reference_t<T>>);

    static auto make_adapters(T& t)
    {
        return transform_elms(t, [](auto&& u)
        {
            auto&& v = unwrap_ref(DSK_FORWARD(u));

            if constexpr(_no_cvref_same_as_<decltype(v), boost::redis::ignore_t>)
            {
                return boost::redis::adapter::detail::ignore_adapter();
            }
            else if constexpr(_tuple_like_<decltype(v)>)
            {
                return redis_tuple_adapter<decltype(v), true>(v);
            }
            else
            {
                return redis_single_adapter<decltype(v)>(v);
            }
        });
    }
    
    DSK_DEF_MEMBER_IF(Nested, size_t) _i = 0; // current tuple element.
    DSK_DEF_MEMBER_IF(Nested, size_t) _n = 0; // nested aggregate element counter.
    decltype(make_adapters(std::declval<T&>())) _adapters;

    explicit redis_tuple_adapter(T& t)
        : _adapters(make_adapters(t))
    {}

    template<class String>
    void operator()(size_t i, boost::redis::resp3::basic_node<String> const& nd, error_code& ec)
    {
        if constexpr(Nested)
        {
            if(nd.depth == 0)
            {
                size_t multiplicity = boost::redis::resp3::element_multiplicity(nd.data_type);
                size_t real_aggr_size = nd.aggregate_size * multiplicity;

                if(real_aggr_size != elm_count<T>)
                {
                    ec = boost::redis::error::incompatible_size;
                }

                return;
            }

            i = _i;

            if(nd.depth == 1 && boost::redis::resp3::is_aggregate(nd.data_type))
            {
                _n = boost::redis::resp3::element_multiplicity(nd.data_type) * nd.aggregate_size;
            }

            if(_n == 0) ++_i;
            else        --_n;
        }

        visit_elm(i, _adapters, [&](auto& adapter)
        {
            adapter(i, nd, ec);
        });
    }

    static constexpr size_t get_supported_response_size() noexcept requires(! Nested)
    {
        return elm_count<T>;
    }
};


} // namespace dsk


namespace boost::redis::adapter::detail{


template<class T>
struct response_traits
{
    static auto adapt(T& v) noexcept
    {
        if constexpr(dsk::_tuple_like_<T>) return dsk::redis_tuple_adapter<T>(v);
        else                               return dsk::redis_single_adapter<T>(v);
    }
};


} // namespace boost::redis::adapter::detail
