#pragma once

#include "utils/PtrMath.h"
#include "utils/Compiler.h"

#include <type_traits>
#include <string_view>

namespace coust {
namespace memory {

size_t constexpr DEFAULT_ALIGNMENT = 16u;

enum Size : size_t {
    byte_8 = 8u,
    byte_16 = 16u,
    byte_32 = 32u,
    byte_64 = 64u,
    byte_128 = 128u,
    byte_256 = 256u,
    byte_512 = 512u,
    kbyte_1 = 1024u,
    kbyte_3 = 3 * 1024u,
    kbyte_5 = 5 * 1024u,
    kbyte_10 = 10 * 1024u,
    kbyte_50 = 50 * 1024u,
};

std::string_view constexpr to_string_view(Size s) {
    switch (s) {
        case byte_8:
            return "byte_8";
        case byte_16:
            return "byte_16";
        case byte_32:
            return "byte_32";
        case byte_64:
            return "byte_64";
        case byte_128:
            return "byte_128";
        case byte_256:
            return "byte_256";
        case byte_512:
            return "byte_512";
        case kbyte_1:
            return "kbyte_1";
        case kbyte_3:
            return "kbyte_3";
        case kbyte_5:
            return "kbyte_5";
        case kbyte_10:
            return "kbyte_10";
        case kbyte_50:
            return "kbyte_50";
    }
    ASSUME(0);
}

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

namespace detail {

template <typename... Class_To_Inherit>
class Pack : public Class_To_Inherit... {};

struct Empty {};

template <typename T>
concept Allocator = requires(T& a) {
    typename T::stateful;
    { a.allocate(size_t{}, size_t{}) } noexcept -> std::same_as<void*>;
    { a.deallocate((void*) nullptr, size_t{}) } noexcept -> std::same_as<void>;
};

template <typename T>
concept GrowableAllocator = Allocator<T> && requires(T& a) {
    { a.grow((void*) nullptr, size_t{}) } noexcept -> std::same_as<void>;
};

}  // namespace detail
}  // namespace memory
}  // namespace coust
