#pragma once

#include <type_traits>

namespace coust {
namespace memory {

inline void* aligned_alloc(size_t size, size_t alignment) noexcept {
    // https://en.cppreference.com/w/cpp/memory/c/aligned_alloc#Notes
    // As an example of the "supported by the implementation" requirement,
    // POSIX function posix_memalign accepts any alignment that is a power of
    // two and a multiple of sizeof(void*), and POSIX-based implementations of
    // aligned_alloc inherit this requirements. Also, this function is not
    // supported in Microsoft Visual C++ because its implementation of
    // std::free() is unable to handle aligned allocations of any kind. Instead,
    // MSVC provides _aligned_malloc (to be freed with _aligned_free).
    alignment = (alignment < sizeof(void*)) ? sizeof(void*) : alignment;
    void* p = nullptr;
#ifdef _MSC_VER
    p = _aligned_malloc(size, alignment);
#else
    p = std::aligned_alloc(size, alignment);
#endif
    return p;
}

inline void aligned_free(void* p) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}

namespace ptr_math {

template <typename P, typename N>

inline P* add(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) + uintptr_t(bytes));
}

template <typename P, typename N>
inline P* sub(P* base, N bytes) noexcept {
    return (P*) (uintptr_t(base) - uintptr_t(bytes));
}

template <typename P>
inline size_t sub(P* l, P* r) noexcept {
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

namespace detail {

template <typename... Class_To_Inherit>
class Pack : public Class_To_Inherit... {};

template <typename T>
concept Allocator = requires(T& a) {
    { a.allocate(size_t{}, size_t{}) } noexcept -> std::same_as<void*>;
    { a.free((void*) nullptr, size_t{}) } noexcept -> std::same_as<void>;
};

}  // namespace detail
}  // namespace memory
}  // namespace coust
