#pragma once

#include <dsk/util/allocator.hpp>
#include <dsk/default_allocator.hpp>
#include <string>


namespace dsk
{
    template<class T, class Tx = std::char_traits<T>, class Al = DSK_DEFAULT_ALLOCATOR<T>>
    using basic_string = std::basic_string<T, Tx, rebind_alloc<Al, T>>;

    using    string = basic_string<char>;
    using   wstring = basic_string<wchar_t>;
    using  u8string = basic_string<char8_t>;
    using u16string = basic_string<char16_t>;
    using u32string = basic_string<char32_t>;
}
