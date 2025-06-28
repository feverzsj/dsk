#pragma once

#include <dsk/config.hpp>
#include <dsk/util/debug.hpp>
#include <dsk/util/ct_vector.hpp>
#include <dsk/util/index_array.hpp>
#include <array>
#include <utility>


namespace dsk{


template<auto Seq, size_t... I>
constexpr decltype(auto) _apply_ct_elms(auto& f, std::index_sequence<I...>)
{
    return f.template operator()<Seq[I]...>();
}

template<auto Seq>
constexpr decltype(auto) apply_ct_elms(auto&& f)
{
    return _apply_ct_elms<Seq>(f, std::make_index_sequence<std::size(Seq)>());
}


template<auto Seq>
constexpr auto transform_ct_elms(auto&& f)
{
    return apply_ct_elms<Seq>([&]<auto... I>()
    {
        return make_index_array(f.template operator()<Seq[I]>()...);
    });
}


template<auto Seq, size_t Lower, size_t Upper>
constexpr auto select_ct_elms()
{
    static_assert(Lower <= Upper);
    static_assert(Upper <= std::size(Seq));

    return apply_ct_elms<make_index_array<Lower, Upper>()>([]<auto... I>()
    {
        return make_index_array(Seq[I]...);
    });
}

template<auto Seq, size_t N>
constexpr auto select_first_n_ct_elms()
{
    return select_ct_elms<Seq, 0, N>();
}

template<auto Seq, size_t N>
constexpr auto select_last_n_ct_elms()
{
    constexpr size_t nElms = std::size(Seq);

    static_assert(N <= nElms);

    return select_ct_elms<Seq, nElms - N, nElms>();
}

template<auto Seq, size_t N>
constexpr auto remove_first_n_ct_elms()
{
    return select_ct_elms<Seq, N, std::size(Seq)>();
}

template<auto Seq, size_t N>
constexpr auto remove_last_n_ct_elms()
{
    constexpr size_t nElms = std::size(Seq);

    static_assert(N <= nElms);

    return select_ct_elms<Seq, 0, nElms - N>();
}


template<auto Seq>
constexpr auto keep_ct_elms(auto pred)
{
    ct_vector<
        std::ranges::range_value_t<decltype(Seq)>,
        std::size(Seq)
    > r;

    for(auto e : Seq)
    {
        if(pred(e))
            r.push_back(e);
    }

    return r;
}


template<auto Seq1, auto Seq2, size_t... I, size_t... J>
constexpr decltype(auto) _apply_ct_elms2(auto& f, std::index_sequence<I...>, std::index_sequence<J...>)
{
    return f(constant_seq<Seq1[I]...>(), constant_seq<Seq2[J]...>());
}

template<auto Seq1, auto Seq2>
constexpr decltype(auto) apply_ct_elms2(auto&& f)
{
    return _apply_ct_elms2<Seq1, Seq2>(f, std::make_index_sequence<std::size(Seq1)>(),
                                          std::make_index_sequence<std::size(Seq2)>());
}


template<auto Seq>
constexpr decltype(auto) foreach_ct_elms(auto&& f)
{
    return apply_ct_elms<Seq>([&]<auto... E>() -> decltype(auto)
    {
        return (... , f.template operator()<E>());
    });
}


template<auto Seq, class EmptyRet = void>
constexpr decltype(auto) foreach_ct_elms_until(auto&& f, auto&& pred, auto&&... onNotFound)
{
    static_assert(sizeof...(onNotFound) <= 1);

    if constexpr(std::size(Seq))
    {
        return apply_ct_elms<Seq>([&]<auto E, auto... Es>(this auto&& self) -> decltype(auto)
        {
            if constexpr(sizeof...(Es) || sizeof...(onNotFound))
            {
                decltype(auto) r = f.template operator()<E>();

                if(! pred(r))
                {
                    if constexpr(sizeof...(Es)) return self.template operator()<Es...>();
                    else                        return (... , may_invoke_with_ts<decltype(r)>(onNotFound));
                }

                return r;
            }
            else
            {
                return f.template operator()<E>();
            }
        });
    }
    else // std::size(Seq) == 0
    {
        if constexpr(sizeof...(onNotFound)) return (... , may_invoke_with_ts<void>(onNotFound));
        else                                return EmptyRet();
    }
}


template<auto Seq, size_t B = 0, size_t N = std::size(Seq)>
constexpr decltype(auto) visit_ct_elm(size_t i, auto&& f)
{
    DSK_ASSERT(/*0 <= i && */i < N);

    static_assert(N > 0);
   
    if constexpr(N <= 16)
    {
    #define _CASE(I)                                                     \
        case I:                                                          \
            if constexpr(I < N)                                          \
                return DSK_FORWARD(f).template operator()<Seq[B + I]>(); \
            else                                                         \
                DSK_UNREACHABLE();                                       \
    /**/
        switch(i)
        {
            _CASE( 0)
            _CASE( 1)
            _CASE( 2)
            _CASE( 3)
            _CASE( 4)
            _CASE( 5)
            _CASE( 6)
            _CASE( 7)
            _CASE( 8)
            _CASE( 9)
            _CASE(10)
            _CASE(11)
            _CASE(12)
            _CASE(13)
            _CASE(14)
            _CASE(15)
        }
    #undef _CASE
        DSK_UNREACHABLE();
    }
    else
    {
        if(i < N/2)
        {
            return visit_ct_elm<Seq, B, N/2>(i, DSK_FORWARD(f));
        }
        else
        {
            return visit_ct_elm<Seq, B + N/2, N - N/2>(i - N/2, DSK_FORWARD(f));
        }
    }
}


} // namespace dsk
