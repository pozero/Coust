#pragma once

namespace ptr_math {

template <typename P, typename N>

inline P* add(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) + uintptr_t(bytes));
}

template <typename P, typename N>
inline P* sub(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) - uintptr_t(bytes));
}

template <typename P, typename U>
inline size_t sub(P* l, U* r) noexcept {
    return (size_t) (uintptr_t(l) - uintptr_t(r));
}

template <typename P>
inline P* align(P* p, size_t alignment) noexcept {
    return (P*) ((uintptr_t(p) + alignment - 1) & (~(alignment - 1)));
}

inline size_t round_up_to_alinged(size_t size, size_t alignment) noexcept {
    return ((size - 1) & ~(alignment - 1)) + alignment;
}

}  // namespace ptr_math
