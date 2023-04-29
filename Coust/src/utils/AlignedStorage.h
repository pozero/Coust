#pragma once

#include "utils/TypeName.h"

namespace coust {

template <typename T>
class AlignedStorage {
public:
    AlignedStorage() noexcept : bytes() {}

    ~AlignedStorage() noexcept {
        if (is_initialized())
            t.~T();
    }

    template <typename... Args>
    T& initialize(Args&&... args) noexcept
        requires(std::constructible_from<T, Args...>)
    {
        T* const p = &t;
        if (!is_initialized()) [[likely]]
            new (p) T{std::forward<Args>(args)...};
        m_initialized = true;
        return *p;
    }

    T& get() noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            fmt::print(stderr, "{} not initialized yet", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        T* const p = &t;
        return *p;
    }

    const T& get() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            fmt::print(stderr, "{} not initialized yet", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        const T* const p = &t;
        return *p;
    }

    T* ptr() noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            fmt::print(stderr, "{} not initialized yet", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        T* const p = &t;
        return p;
    }

    const T* ptr() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            fmt::print(stderr, "{} not initialized yet", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        const T* const p = &t;
        return p;
    }

    bool is_initialized() const noexcept { return m_initialized; }

private:
    static constexpr size_t round_up_to_alinged(
        size_t size, size_t alignment) noexcept {
        return ((size - 1) & ~(alignment - 1)) + alignment;
    }

    union {
        char bytes[round_up_to_alinged(sizeof(T), alignof(T))];
        T t;
    };
    bool m_initialized = false;
};

}  // namespace coust
