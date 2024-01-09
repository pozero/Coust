#pragma once

#include <span>

#include "utils/allocators/StlContainer.h"

namespace coust {

template <typename T, typename U, typename A>
std::span<T> to_span(memory::vector<U, A> const& vec) {
    return std::span{(T*) (vec.data()), vec.size() * sizeof(U)};
}

}  // namespace coust
