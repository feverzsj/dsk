#pragma once

#include <dsk/util/stringify.hpp>
#include <dsk/util/base64.hpp>
#include <dsk/util/strlit.hpp>
#include <dsk/jser/cpo.hpp>


namespace dsk{


// escape " or \ or / or control-character as \u four-hex-digits
constexpr char* append_escaped_jstr(char* d, char const* s, size_t n)
{
    static constexpr char const hexDigits[16] ={
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };

    static constexpr char const escape[256] ={
    #define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        //   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'b', 't', 'n', 'u', 'f', 'r', 'u', 'u', // 00
        'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', // 10
        0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20
        Z16, Z16,                                                                       // 30~4F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, // 50
        Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16, Z16                                // 60~FF
    #undef Z16
    };

    *(d++) = '"';

    for(size_t i = 0; i < n; ++i)
    {
        char const c = s[i];
        unsigned char const uc = c;

        if(char const ec = escape[uc])
        {
            *(d++) = '\\';
            *(d++) = ec;
            if(ec == 'u')
            {
                *(d++) = '0';
                *(d++) = '0';
                *(d++) = hexDigits[uc >> 4];
                *(d++) = hexDigits[uc & 0xF];
            }
        } else
        {
            *(d++) = c;
        }
    }

    *(d++) = '"';

    return d;
}

constexpr void append_escaped_jstr(_resizable_byte_buf_ auto& b, _byte_str_ auto const& v)
{
    size_t len = str_size(v);
    size_t maxLen = 2 + len * 6; // suppose all to be escaped "\uxxxx..."

    char* e = append_escaped_jstr(buy_buf<char>(b, maxLen), str_data<char>(v), len);
    
    resize_buf(b, e - buf_data<char>(b));
}


enum json_escape_key_e
{
    json_escape_key   = true,
    json_noescape_key = false
};


template<
    _resizable_byte_buf_ Buf,
    auto              Delimiter = '\n', // or strlit("multi character delimiter")
    json_escape_key_e EscapeKey = json_noescape_key,
    class             Derived = void
>
class json_writer
{
    using ch_type = buf_val_t<Buf>;

    Buf& _b;

    constexpr auto* derived() noexcept
    {
        if constexpr(_void_<Derived>) return this;
        else                          return static_cast<Derived*>(this);
    }

public:
    constexpr explicit json_writer(Buf& b)
        : _b(b)
    {}

    constexpr void put(_byte_ auto c)
    {
        _b.push_back(static_cast<ch_type>(c));
    }

    constexpr void put(_byte_str_ auto const& s)
    {
        append_str(_b, s);
    }

    constexpr void amend_last_if_eq_to(_byte_ auto t, _byte_ auto a)
    {
        auto& u = buf_back(_b);

        if(u == static_cast<ch_type>(t)) u = static_cast<ch_type>(a);
        else                             put(a);
    }

    constexpr void key(_byte_str_ auto const& s)
    {
        if constexpr(EscapeKey) derived()->str(s);
        else                    derived()->str_noescape(s);
    }

    constexpr void fp(std::floating_point auto v)
    {
        append_as_str(_b, v);
    }

    constexpr void integer(_non_bool_integral_ auto v)
    {
        append_as_str(_b, v);
    }

    using is_jwriter = void;

    constexpr void null() { put("null"); }

    template<_arithmetic_ T>
    constexpr void arithmetic(T v)
    {
             if constexpr(std::    is_same_v<T, bool>) put(v ? "true" : "false");
        else if constexpr(std::is_floating_point_v<T>) derived()->fp(v);
        else                                           derived()->integer(v);
    }

    // escape " or \ or / or control-character as \u four-hex-digits
    constexpr void str(_byte_str_ auto const& v)
    {
        append_escaped_jstr(_b, v);
    }

    constexpr void str_noescape(_byte_str_ auto const& v)
    {
        //append_str(_b, "\"", v, "\"");

        size_t n = str_size(v);
        char*  d = buy_buf<char>(_b, 2 + n);

        d[0] = '"';
        std::copy_n(str_data<char>(v), n, d + 1);
        d[1 + n] = '"';
    }

    constexpr void binary(auto const& v)
    {
        put('"');
        append_as_base64(_b, v);
        put('"');
    }


    constexpr void arr_scope(uint32_t /*n*/, auto&& f)
    {
        put('[');
        f();
        amend_last_if_eq_to(',', ']');
    }

    constexpr void elm_scope(auto&& ef)
    {
        ef();
        put(',');
    }

    constexpr void obj_scope(size_t /*maxCnt*/, auto&& f)
    {
        put('{');
        f();
        amend_last_if_eq_to(',', '}');
    }

    constexpr void fld_scope(auto const& k, auto&& vf)
    {
        static_assert(_jkey_<decltype(k)> || _byte_str_<decltype(k)> || _arithmetic_<decltype(k)>,
                      "unsupported key type");

        key(stringify(jkey_str_or_as_is(k)));
        put(':');
        vf();
        put(',');
    }

    void write_delimiter()
    {
        put(Delimiter);
    }
};


template<
    auto              Delimiter = '\n', // or strlit("multi character delimiter")
    json_escape_key_e EscapeKey = json_noescape_key
>
constexpr auto make_json_writer(_resizable_byte_buf_ auto& b) noexcept
{
    return json_writer<DSK_NO_CVREF_T(b), Delimiter, EscapeKey>(b);
}


template<
    json_escape_key_e EscapeKey = json_noescape_key
>
constexpr void write_json(auto&& def, auto const& v, _resizable_byte_buf_ auto& b)
{
    jwrite(def, v, make_json_writer<'\n', EscapeKey>(b));
}

template<
    auto              Delimiter = '\n', // or strlit("multi character delimiter")
    json_escape_key_e EscapeKey = json_noescape_key
>
constexpr void write_json_range(auto&& def, std::ranges::range auto const& vs, _resizable_byte_buf_ auto& b)
{
    jwrite_range(def, vs, make_json_writer<Delimiter, EscapeKey>(b));
}


} // namespace dsk
