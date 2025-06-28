#pragma once

#include <dsk/util/tuple.hpp>


namespace dsk{


template<class... P>
class make_params : private tuple<P...>
{
    template<class Tag>
    static constexpr typename Tag::__ERROR__parameter_not_found _get();

    template<class Tag, class E0, class... E>
    static constexpr decltype(auto) _get(E0& e0 [[maybe_unused]], E&... e [[maybe_unused]])
    {
        if constexpr(requires{ e0(Tag{}); }) return e0(Tag{});
        else                                 return _get<Tag>(e...);
    }

public:
    using tuple::tuple;

    template<class Tag>
    constexpr decltype(auto) operator()(Tag) const
    {
        return apply_elms(

            static_cast<tuple const&>(*this),

            [](auto&... e) -> decltype(auto)
            {
                return _get<Tag>(e...);
            }
        );
    }

    template<class Tag>
    static consteval bool has(Tag)
    {
        return DSK_REQ_EXP(... || requires{ std::declval<P>()(Tag()); });
    }
};

template<class... P>
make_params(P...)->make_params<P...>;


// usage:
// 
// void foo(auto&&... p)
// {
//     // put override options first, put default options last.
//     // options not present in override_options/default_options are mandatory, otherwise optional.
//     // use params.has(tag) to detect if an option exists
//     [constexpr] auto params = make_params(override_options..., p..., default_options...);
//      ^^^^^^^^^ if all p are constexpr.
//     [constexpr] auto o = params(option_tag);
//     ^^^^^^^^^ if this option is constexpr.
// }
// 
// inline constexpr struct t_tag_t{} t_tag;
// inline constexpr auto p_tag = [](Tag){ return ...; };
// foo(p_tag(...), ...)
// 


// generate a param for Tag that forwards v
template<class Tag>
inline constexpr auto p_forward = []<class T>(T&& v) { return [&v](Tag) -> T&& { return static_cast<T&&>(v); }; };

// generate a param for Tag that return v
template<class Tag, auto V>
inline constexpr auto p_constant = [](Tag){ return V; };


template<bool Cond>
constexpr auto param_if(auto&& p)
{
    struct t_dummy_param_t{};

    if constexpr(Cond)
        return p;
    else
        return [](t_dummy_param_t){};
}


} // namespace dsk
