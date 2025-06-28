#pragma once

#include <dsk/util/concepts.hpp>
#include <cmath>


namespace dsk
{


struct close_options
{
    enum method_e
    {
        asymmetric, // the b value is used for scaling the tolerance
        strong,     // the tolerance is scaled by the smaller of the two values
        weak,       // the tolerance is scaled by the larger of the two values
        average     // the tolerance is scaled by the average of the two values
    };

    double rel_tol = 1e-09;
    double abs_tol = 0;
    method_e method = weak;

    constexpr close_options rebind(method_e m) const noexcept
    {
        close_options t = *this;
        t.method = m;
        return t;
    }
};


// https://peps.python.org/pep-0485/
// https://docs.python.org/3/library/math.html#math.isclose
// https://github.com/PythonCHB/close_pep/blob/master/is_close.py
// https://github.com/python/cpython/blob/main/Modules/mathmodule.c#L3146
// https://www.boost.org/doc/libs/1_35_0/libs/test/doc/components/test_tools/floating_point_comparison.html
template<
    close_options Opts = {}
>
constexpr bool isclose(_fp_ auto a_, _fp_ auto b_) noexcept
{
    using ftype = std::common_type_t<decltype(a_), decltype(b_)>;

    // if tols cannot be represented in ftype, you should use wider a_/b_.
    constexpr ftype rel_tol = Opts.rel_tol;
    constexpr ftype abs_tol = Opts.abs_tol;

    static_assert(rel_tol >= 0 && abs_tol >= 0);

    ftype a = a_;
    ftype b = b_;

    // short circuit exact equality -- needed to catch two infinities of
    // the same sign. And perhaps speeds things up a bit sometimes.
    if(a == b)
    {
        return true;
    }

    // This catches the case of two infinities of opposite sign, or
    // one infinity and one finite number. Two infinities of opposite
    // sign would otherwise have an infinite relative tolerance.
    // Two infinities of the same sign are caught by the equality check above.
    if(std::isinf(a) || std::isinf(b))
    {
        return false;
    }

    ftype diff = std::fabs(b - a);

    if constexpr(Opts.method == close_options::asymmetric)
    {
        if constexpr(abs_tol) return diff <= std::fabs(rel_tol * b) || diff <= abs_tol;
        else                  return diff <= std::fabs(rel_tol * b);
    }
    else if constexpr(Opts.method == close_options::strong)
    {
        if constexpr(abs_tol) return (diff <= std::fabs(rel_tol * b) && diff <= std::fabs(rel_tol * a)) || diff <= abs_tol;
        else                  return  diff <= std::fabs(rel_tol * b) && diff <= std::fabs(rel_tol * a);
    }
    else if constexpr(Opts.method == close_options::weak)
    {
        if constexpr(abs_tol) return diff <= std::fabs(rel_tol * b) || diff <= std::fabs(rel_tol * a) || diff <= abs_tol;
        else                  return diff <= std::fabs(rel_tol * b) || diff <= std::fabs(rel_tol * a);
    }
    else if constexpr(Opts.method == close_options::average)
    {
        if constexpr(abs_tol) return diff <= std::fabs(rel_tol * (a + b) / 2) || diff <= abs_tol;
        else                  return diff <= std::fabs(rel_tol * (a + b) / 2);
    }
    else
    {
        static_assert(false, "unknown method");
    }
}


template<
    close_options Opts = {}
>
constexpr bool isclose_to(_fp_ auto a_, _fp_ auto expected_) noexcept
{
    return isclose<Opts.rebind(close_options::asymmetric)>(a_, expected_);
}


} // namespace dsk
