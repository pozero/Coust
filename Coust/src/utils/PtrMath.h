#pragma once

namespace ptr_math {

template <typename P, typename N>
inline constexpr P* add(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) + uintptr_t(bytes));
}

template <typename P, typename N>
inline constexpr P* sub(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) - uintptr_t(bytes));
}

template <typename P, typename U>
inline constexpr size_t sub(P* l, U* r) noexcept {
    return (size_t) (uintptr_t(l) - uintptr_t(r));
}

template <typename P>
inline constexpr P* align(P* p, size_t alignment) noexcept {
    return (P*) ((uintptr_t(p) + alignment - 1) & (~(alignment - 1)));
}

inline constexpr size_t round_up_to_alinged(
    size_t size, size_t alignment) noexcept {
    return ((size - 1) & ~(alignment - 1)) + alignment;
}

inline bool is_aligned(void* p, size_t alignment) noexcept {
    return ((uintptr_t) p & (alignment - 1)) == 0;
}

inline constexpr bool is_aligned(size_t size, size_t alignment) noexcept {
    return (size & (alignment - 1)) == 0;
}

}  // namespace ptr_math
