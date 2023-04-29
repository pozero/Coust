#pragma once

#include "utils/Compiler.h"

#include <type_traits>

namespace coust {

template <typename E>
constexpr bool has(E lhs, E rhs) noexcept
    requires(std::is_enum_v<E> && std::is_integral_v<std::underlying_type_t<E>>)
{
    using N = std::underlying_type_t<E>;
    return ((N(lhs) & N(rhs)) != N(0));
}

WARNING_PUSH
#if defined(_MSC_VER) and !defined(__clang__)
    #pragma warning(disable : 4702)
#endif
template <typename E, typename... REST>
constexpr E merge(E first, E second, REST... rest) noexcept
    requires(std::is_enum_v<E> &&
             std::is_integral_v<std::underlying_type_t<E>> &&
             std::conjunction_v<std::is_same<E, REST>...>)
{
    using N = std::underlying_type_t<E>;
    E const headRes = (E) ((N) first | (N) second);
    if constexpr (sizeof...(REST) > 0) {
        return merge(headRes, rest...);
    }
    return headRes;
}
WARNING_POP

}  // namespace coust
