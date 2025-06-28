#pragma once

#include <dsk/config.hpp>


namespace dsk{


struct decompr_r
{
    size_t nIn   = 0;     // used (compressed) input
    size_t nOut  = 0;     // appended (decompressed) output
    bool   isEnd = false; // end of stream reached at nIn (of input).
};


} // namespace dsk
