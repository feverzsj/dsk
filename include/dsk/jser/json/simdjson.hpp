#pragma once

#include <dsk/jser/cpo.hpp>
#include <dsk/jser/err.hpp>
#include <dsk/util/ct.hpp>
#include <dsk/util/tuple.hpp>
#include <dsk/util/base64.hpp>
#include <simdjson.h>


namespace dsk{


class simdjson_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "simdjson"; }

    std::string message(int condition) const override
    {
        switch(static_cast<simdjson::error_code>(condition))
        {
            case simdjson::CAPACITY                   : return "This parser can't support a document that big";
            case simdjson::MEMALLOC                   : return "Error allocating memory, most likely out of memory";
            case simdjson::TAPE_ERROR                 : return "Something went wrong, this is a generic error";
            case simdjson::DEPTH_ERROR                : return "Your document exceeds the user-specified depth limitation";
            case simdjson::STRING_ERROR               : return "Problem while parsing a string";
            case simdjson::T_ATOM_ERROR               : return "Problem while parsing an atom starting with the letter 't'";
            case simdjson::F_ATOM_ERROR               : return "Problem while parsing an atom starting with the letter 'f'";
            case simdjson::N_ATOM_ERROR               : return "Problem while parsing an atom starting with the letter 'n'";
            case simdjson::NUMBER_ERROR               : return "Problem while parsing a number";
            case simdjson::BIGINT_ERROR               : return "The integer value exceeds 64 bits";
            case simdjson::UTF8_ERROR                 : return "the input is not valid UTF-8";
            case simdjson::UNINITIALIZED              : return "unknown error, or uninitialized document";
            case simdjson::EMPTY                      : return "no structural element found";
            case simdjson::UNESCAPED_CHARS            : return "found unescaped characters in a string";
            case simdjson::UNCLOSED_STRING            : return "missing quote at the end";
            case simdjson::UNSUPPORTED_ARCHITECTURE   : return "unsupported architecture";
            case simdjson::INCORRECT_TYPE             : return "JSON element has a different type than user expected";
            case simdjson::NUMBER_OUT_OF_RANGE        : return "JSON number does not fit in 64 bits";
            case simdjson::INDEX_OUT_OF_BOUNDS        : return "JSON array index too large";
            case simdjson::NO_SUCH_FIELD              : return "JSON field not found in object";
            case simdjson::IO_ERROR                   : return "Error reading a file";
            case simdjson::INVALID_JSON_POINTER       : return "Invalid JSON pointer syntax";
            case simdjson::INVALID_URI_FRAGMENT       : return "Invalid URI fragment";
            case simdjson::UNEXPECTED_ERROR           : return "indicative of a bug in simdjson";
            case simdjson::PARSER_IN_USE              : return "parser is already in use";
            case simdjson::OUT_OF_ORDER_ITERATION     : return "tried to iterate an array or object out of order (checked when SIMDJSON_DEVELOPMENT_CHECKS=1)";
            case simdjson::INSUFFICIENT_PADDING       : return "The JSON doesn't have enough padding for simdjson to safely parse it";
            case simdjson::INCOMPLETE_ARRAY_OR_OBJECT : return "The document ends early";
            case simdjson::SCALAR_DOCUMENT_AS_VALUE   : return "A scalar document is treated as a value";
            case simdjson::OUT_OF_BOUNDS              : return "Attempted to access location outside of document";
            case simdjson::TRAILING_CONTENT           : return "Unexpected trailing content in the JSON input";
            default: break; // for unhandled enums
        }

        return "undefined";
    }
};

inline constexpr simdjson_err_category g_simdjson_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(simdjson, error_code, dsk::g_simdjson_err_cat)


namespace dsk{


template<class T> void _simdjson_result_impl(simdjson::simdjson_result<T>&);
template<class T> concept _simdjson_result_ = requires(std::remove_cvref_t<T>& t){ _simdjson_result_impl(t); };


constexpr bool dsk_has_err(has_err_cpo, _simdjson_result_ auto const& t) noexcept
{
    return static_cast<bool>(t.error());
}

constexpr decltype(auto) dsk_get_err(get_err_cpo, _simdjson_result_ auto&& t) noexcept
{
    return DSK_FORWARD(t).error();
}

constexpr decltype(auto) dsk_get_val(get_val_cpo, _simdjson_result_ auto&& t) noexcept
{
    return DSK_FORWARD(t).value_unsafe();
}

template<class T> auto dsk_val_t(val_t_cpo, simdjson::simdjson_result<T> const&) -> T;


template<class T>
using simdjson_expected = expected<T, simdjson::error_code>;


// The reader should be reused to avoid frequent allocation.
class simdjson_reader
{
    simdjson::ondemand::parser _p;

public:
    using is_jreader = void;

    static constexpr auto padding = simdjson::SIMDJSON_PADDING;
    static constexpr auto default_max_doc_size = simdjson::ondemand::DEFAULT_BATCH_SIZE;

    auto& parser() noexcept { return _p; }

    // Find the beginning of likely json document start at `offset`.
    // If not found, return npos.
    static size_t find_doc(_byte_buf_ auto const& input, size_t offset) noexcept
    {
        return str_view<char>(input).find_first_of("{[", offset);
    }

    // NOTE: all read methods need padded input buffer.
    
    template<_byte_buf_ B>
    struct input_manager
    {
        static constexpr bool isPaddedStr = _no_cvref_same_as_<B, simdjson::padded_string>;
        static constexpr bool isPaddedStrView = _no_cvref_same_as_<B, simdjson::padded_string_view>;
        static constexpr bool isPadded = isPaddedStr || isPaddedStrView;
        static constexpr bool isResizable = _resizable_buf_<B>;
        static constexpr bool isReservable = _reservable_buf_<B>;

        static_assert(isPadded || isResizable);

        B& b;
        size_t len = 0, cap = 0, offset = 0;

        input_manager(auto&& b_, size_t offset_)
            : b(b_), offset(offset_)
        {
            if constexpr(isPaddedStrView)
            {
                DSK_ASSERT(b.padding() >= padding);
            }

            len = buf_size(b);
            cap = len + padding;

            DSK_ASSERT(len >= offset);

                 if constexpr(! isPadded && isReservable) reserve_buf(b, cap);
            else if constexpr(! isPadded && isResizable ) resize_buf(b, cap);
        }

        ~input_manager()
        {
            if constexpr(! isPadded && ! isReservable && isResizable)
            {
                resize_buf(b, len); // resize back
            }
        }

        auto padded_buf() const noexcept
        {
            return simdjson::padded_string_view(buf_data<char>(b) + offset,
                                                len - offset,
                                                cap - offset);
        }
    };

    // returns offset after last read char.
    expected<size_t> read(_byte_buf_ auto&& b, auto&& f, size_t offset = 0)
    {
        input_manager<decltype(b)> im(b, offset);

        DSK_U_TRY_FWD(doc, _p.iterate(im.padded_buf()));

        DSK_E_TRY_ONLY(f(doc));

        if(! doc.at_end()) // there is trailing content
        {
            DSK_E_TRY_FWD(e, doc.current_location());
            return static_cast<size_t>(e - buf_data<char>(b));
        }

        return im.len;
    }

    // b: some sort of delimited/concatenated json. Shouldn't be comma delimited, as it's inefficient.
    // maxDocSize: max size of single json document.
    // returns offset after last read char.
    expected<size_t> read_foreach(_byte_buf_ auto&& b, auto&& f, size_t offset = 0,
                                  size_t maxDocSize = default_max_doc_size, bool commaSeparated = false)
    {
        input_manager<decltype(b)> im(b, offset);
        
        auto&& pb = im.padded_buf();

        DSK_E_TRY_FWD(docs, _p.iterate_many(pb.data(), pb.size(), maxDocSize, commaSeparated));

        auto it = docs.begin(), end = docs.end();

        for(; it != end; ++it)
        {
            DSK_E_TRY_FWD(doc, *it);
            
            DSK_E_TRY(bool go_on, f(doc));
            
            if(! go_on)
            {
                break;
            }
        }

        if(it != end)
        {
            ++it;
            DSK_E_TRY_ONLY(it.error());
        }

        //if(it != end) return buf_data(b) - it.source().data(); // offset to next doc's begin
        if(it != end) return offset + it.current_index(); // offset of next doc's begin
        else          return im.len - docs.truncated_bytes();
    }
};


template<class T>
concept _simdjson_d_or_v_ = _no_cvref_one_of_<T, simdjson::ondemand::value,
                                                 simdjson::ondemand::document,
                                                 simdjson::ondemand::document_reference>;


simdjson::error_code dsk_get_jstr(get_jstr_cpo, simdjson::ondemand::value& v, auto& t) noexcept
{
    DSK_E_TRY_FWD(s, v.get_string());
    assign_buf(t, s);
    return {};
}

jerrc dsk_get_jstr_noescape(get_jstr_noescape_cpo, simdjson::ondemand::value& v, auto& t) noexcept
{
    // v.get_raw_json_string() won't give size.
    // v.raw_json() will consume the whole value.
    if(auto s = v.raw_json_token(); s.starts_with('"'))
    {
        if(auto q = s.rfind('"'); q != 0)
        {
            assign_buf(t, s.data() + 1, q - 1);
            return {};
        }
    }

    return jerrc::not_a_string;
}

error_code dsk_get_jbinary(get_jbinary_cpo, simdjson::ondemand::value& v, auto& t) noexcept
{
    std::string_view s;
    DSK_E_TRY_ONLY(get_jstr_noescape(v, s));
    return from_base64(s, t);
}

template<class T>
simdjson::error_code dsk_get_jarithmetic(get_jarithmetic_cpo, simdjson::ondemand::value& v, T& t) noexcept
{
    static_assert(std::is_arithmetic_v<T>);

    DSK_E_TRY_FWD(a, [&]()
    {
             if constexpr(std::    is_same_v<T, bool>) return v.get_bool();
        else if constexpr(std::is_floating_point_v<T>) return v.get_double();
        else if constexpr(std::        is_signed_v<T>) return v.get_int64();
        else                                           return v.get_uint64();
    }());

    if constexpr(sizeof(a) > sizeof(T))
    {
        if(DSK_UNLIKELY(a > std::numeric_limits<T>::max()))
        {
            return simdjson::NUMBER_OUT_OF_RANGE;
        }

        if constexpr(std::is_signed_v<T>)
        {
            if(DSK_UNLIKELY(a < std::numeric_limits<T>::lowest()))
            {
                return simdjson::NUMBER_OUT_OF_RANGE;
            }
        }
    }

    t = static_cast<T>(a);
    return {};
}

// simdjson_result<bool>
auto dsk_check_jnull(check_jnull_cpo, simdjson::ondemand::value& v) noexcept
{
    return v.is_null();
}

// simdjson_result<ondemand::object>
auto dsk_get_jobj(get_jobj_cpo, _simdjson_d_or_v_ auto& v) noexcept
{
    return v.get_object();
}

auto& dsk_get_jobj(get_jobj_cpo, simdjson::ondemand::object& o) noexcept
{
    return o;
}

template<class Iter, class T>
struct simdjson_iter
{
    Iter it;

    explicit operator bool() const noexcept
    {
        return it != it;
    }

    // must be called in each iteration
    // simdjson_result<T>
    auto operator*() noexcept
    {
        return *it;
    }

    // must be called in each iteration, after operator*() and any use of the returned value.
    simdjson_iter& operator++() noexcept
    {
        ++it;
        return *this;
    }

    // shouldn't use this iterator after calling this method.
    simdjson::error_code skip_remains()
    {
        if(it != it)
        {
            ++it; // assume current is always used
        }

        while(it != it)
        {
            DSK_E_TRY_ONLY(*it); // for detecting error and sometimes necessary consumption.
            ++it;
        }

        return {};
    }
//     void skip_remains()
//     {
//         while(it != it)
//         {
//             ++it;
//         }
//     }
};

using simdjson_obj_iter = simdjson_iter<simdjson::ondemand::object_iterator, simdjson::ondemand::field>;
using simdjson_arr_iter = simdjson_iter<simdjson::ondemand::array_iterator, simdjson::ondemand::value>;


simdjson_expected<simdjson_arr_iter> dsk_get_jarr_iter(get_jarr_iter_cpo, _simdjson_d_or_v_ auto& v) noexcept
{
    DSK_E_TRY_FWD(beg, v.begin());

    return simdjson_arr_iter{beg};
}

template<class Key>
error_code dsk_foreach_jfld(foreach_jfld_cpo<Key>, simdjson::ondemand::object& o, auto&& h)
{
    DSK_E_TRY_FWD(it, o.begin());

    for(; it != it; ++it)
    {
        DSK_E_TRY_FWD(fld, *it);

        if constexpr(_str_<Key>)
        {
            DSK_E_TRY_ONLY(h(Key(fld.escaped_key()), fld.value()));
        }
        else if constexpr(_arithmetic_<Key>)
        {
            DSK_E_TRY_FWD(k, str_to<Key>(fld.escaped_key()));
            DSK_E_TRY_ONLY(h(k, fld.value()));
        }
        else
        {
            static_assert(false, "unsupported type");
        }
    }

    return {};
}


struct simdjson_loop_iter
{
    simdjson::ondemand::object& obj;
    simdjson::ondemand::object_iterator it;
    unsigned n = 0;
    unsigned j = 0; // relative pos
    bool counting = true; // counting n in first pass

    // start a new loop relative to current position.
    void start()
    {
        DSK_ASSERT(counting || n);
        
        if(j)
        {
            ++it;
        }

        j = 0;
    }

    // must use the returned non null value.
    std::optional<decltype(*it)> next() noexcept
    {
        if(j)
            ++it;

        if(counting)
        {
            if(it != it)
            {
                ++n;
                ++j;
                return *it;
            }

            counting = false;
        }

        if(j++ < n)
        {
            if(it == it)
            {
                DSK_E_TRY_ONLY(obj.reset());
                DSK_E_TRY(it, obj.begin());
            }

            return *it;
        }

        return std::nullopt;
    }

    // shouldn't use this iterator after calling this method.
    simdjson::error_code skip_remains()
    {
        if(it != it)
        {
            ++it; // assume current is always used
        }

        while(it != it)
        {
            DSK_E_TRY_ONLY(*it); // for detecting error and sometimes necessary consumption.
            ++it;
        }

        return {};
    }
};

inline simdjson_expected<simdjson_loop_iter> dsk_get_jval_finder(get_jval_finder_cpo, simdjson::ondemand::object& o) noexcept
{
    DSK_E_TRY_FWD(beg, o.begin());
    return simdjson_loop_iter{o, beg};
}

simdjson_expected<simdjson_loop_iter> dsk_get_jval_finder(get_jval_finder_cpo, _simdjson_d_or_v_ auto& v) noexcept
{
    DSK_E_TRY_FWD(o, v.get_object());
    return dsk_get_jval_finder(get_jval_finder_cpo(), o);
}

inline auto& dsk_get_jval_finder(get_jval_finder_cpo, simdjson_loop_iter& v) noexcept
{
    return v;
}

// NOTE: by default, keys should be unescaped/raw
expected<int> dsk_foreach_found_jval(foreach_found_jval_cpo, simdjson_loop_iter& it, auto& mks, auto&& h)
{
    static_assert(mks.count);

    int unmatched = mks.count;

    it.start();

    while(auto ov = it.next())
    {
        DSK_E_TRY_FWD(DSK_PARAM([fk, fv]), *ov);

        auto ec = foreach_elms_until_err(mks, [&]<auto I>(auto& mk) -> error_code
        {
            auto&[matched, k] = mk;

            if(! matched && fk == jkey_str(k))
            {
                DSK_U_TRY_ONLY(h.template operator()<I>(fv));
                    
                matched = true;

                --unmatched;

                return errc::none_err; // for matched
            }

            return {};
        });

        if(ec && ec != errc::none_err)
        {
            return ec;
        }

        if(! unmatched)
        {
            return 0;
        }
    }

    return unmatched;
}

template<template<class...> class L, class... T>
simdjson_expected<unsigned> dsk_get_jval_matched_type(get_jval_matched_type_cpo,
                                                      simdjson::ondemand::value& v, L<T...> const&) noexcept
{
    static_assert(sizeof...(T));
    constexpr auto   ts = type_list<T...>;
    constexpr auto ints = ct_keep(ts, DSK_TYPE_EXP_OF(U, _integral_<U> && ! _same_as_<U, bool>));
    constexpr auto  fps = ct_keep(ts, DSK_TYPE_EXP_OF(U, _fp_<U>));
    static_assert(ints.size() <= 1 && fps.size() <= 1, "At most 1 integral and 1 floating point types are allowed for number.");
    // NOTE: for numbers, if 2 types are present, they should have same size, but may not decode to same type.

    DSK_E_TRY_FWD(t, v.type());
    
    if constexpr(ints.size() || fps.size())
    {
        if(t == simdjson::ondemand::json_type::number)
        {
                 if constexpr(ints. size() && fps.empty()) return ints[0];
            else if constexpr(ints.empty() && fps. size()) return  fps[0];
            else           // ints. size() && fps. size()
            {
                static_assert(sizeof(type_pack_elm<ints[0], T...>) == sizeof(type_pack_elm<fps[0], T...>));

                DSK_E_TRY_FWD(isInt, v.is_integer());
                return isInt ? ints[0] : fps[0];
            }
        }
    }

    constexpr auto sts = ct_remove(ts, DSK_TYPE_EXP_OF(U, _arithmetic_<U> && ! _same_as_<U, bool>));

    return foreach_ct_elms_until<sts>
    (
        [&]<unsigned I>() -> simdjson_expected<unsigned>
        {
            using e_t = type_pack_elm<I, T...>;

            if constexpr(_same_as_<e_t, std::monostate>)
            {
                static_assert(I == 0);
                if(t == simdjson::ondemand::json_type::null) return I;
            }
            else if constexpr(_same_as_<e_t, bool>)
            {
                if(t == simdjson::ondemand::json_type::boolean) return I;
            }
            else if constexpr(_str_<e_t>)
            {
                if(t == simdjson::ondemand::json_type::string) return I;
            }
            else if constexpr(_jobj_<e_t>)
            {
                if(t == simdjson::ondemand::json_type::object) return I;
            }
            else if constexpr(_tuple_like_<e_t> || std::ranges::range<e_t>)
            {
                if(t == simdjson::ondemand::json_type::array) return I;
            }
            else
            {
                static_assert(false, "unsupported type");
            }

            return simdjson::INCORRECT_TYPE;
        },
        [](auto& r)
        {
            return has_val(r);
        }
    );
}


} // namespace dsk
