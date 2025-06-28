#pragma once

#include <dsk/util/str.hpp>
#include <dsk/util/handle.hpp>
#include <libstemmer.h>


namespace dsk{


class snowball_stemmer : public unique_handle<sb_stemmer*, sb_stemmer_delete>
{
public:
    snowball_stemmer() = default;
    snowball_stemmer(_byte_str_ auto const& algo, _byte_str_ auto const& enc)
        : unique_handle<sb_stemmer*, sb_stemmer_delete>(
            sb_stemmer_new(cstr_data<char>(as_cstr(algo)), cstr_data<char>(as_cstr(enc))))
    {}

    explicit snowball_stemmer(_byte_str_ auto const& algo)
        : snowball_stemmer(algo, "UTF_8")
    {}

    template<_byte_str_ T = std::string_view>
    T stem(_byte_str_ auto const& word)
    {
        // must call sb_stemmer_stem() before sb_stemmer_length();
        auto* s = sb_stemmer_stem(checked_handle(), str_data<sb_symbol>(word), str_size<int>(word));

        return T(str_data<str_val_t<T>>(s), sb_stemmer_length(handle()));
    }
};


} // namespace std