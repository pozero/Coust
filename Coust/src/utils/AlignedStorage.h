#pragma once

#include <mutex>

#include "utils/TypeName.h"
#include "utils/Compiler.h"

namespace coust {

template <typename T>
class AlignedStorage {
public:
    AlignedStorage() noexcept : bytes() {}

    ~AlignedStorage() noexcept {
        if (is_initialized())
            std::destroy_at(&t);
    }

    template <typename... Args>
    T& initialize(Args&&... args) noexcept
        requires(std::constructible_from<T, Args...>)
    {
        T* const p = &t;
        // static intialization can't work here, since we need to call the ctor
        // once per obejct instead of per instantiation of this template class
        std::call_once(m_init_flag, std::construct_at<T, Args...>, p,
            std::forward<Args>(args)...);
        m_initialized = true;
        return *p;
    }

    T& get() noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        T* const p = &t;
        return *p;
    }

    const T& get() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        const T* const p = &t;
        return *p;
    }

    T* ptr() noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        T* const p = &t;
        return p;
    }

    const T* ptr() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        const T* const p = &t;
        return p;
    }

    bool is_initialized() const noexcept { return m_initialized; }

private:
    union {
        alignas(T) char bytes[sizeof(T)];
        T t;
    };
    std::once_flag m_init_flag{};
    bool m_initialized = false;
};

}  // namespace coust
