#pragma once

#include <type_traits>


namespace dsk{


template<class T>
concept _scheduler_ = requires{ typename std::remove_cvref_t<T>::is_scheduler; };


} // namespace dsk
