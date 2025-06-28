#pragma once

#include <ratio>
#include <chrono>


namespace dsk{


using  B_ = std::chrono::duration<long long, std::ratio<1>>; // byte
using KiB = std::chrono::duration<long long, std::ratio<1024>>;
using MiB = std::chrono::duration<long long, std::ratio<1024LL * 1024LL>>;
using GiB = std::chrono::duration<      int, std::ratio<1024LL * 1024LL * 1024LL>>;
using TiB = std::chrono::duration<      int, std::ratio<1024LL * 1024LL * 1024LL * 1024LL>>;
using PiB = std::chrono::duration<      int, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL>>;
using EiB = std::chrono::duration<      int, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>>;
// overflowed
// using ZiB = std::chrono::duration<long long, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>>;
// using YiB = std::chrono::duration<long long, std::ratio<1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL>>;


using Ones  = std::chrono::duration<long long, std::ratio<1>>;
using Deca  = std::chrono::duration<long long, std::deca>;
using Hecto = std::chrono::duration<long long, std::hecto>;
using Kilo  = std::chrono::duration<long long, std::kilo>;
using Mega  = std::chrono::duration<long long, std::mega>;
using Giga  = std::chrono::duration<      int, std::giga>;
using Tera  = std::chrono::duration<      int, std::tera>;
using Peta  = std::chrono::duration<      int, std::peta>;
using Exa   = std::chrono::duration<      int, std::exa>;


} // namespace dsk
