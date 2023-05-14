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
        T* const p = std::addressof(t);
        // static intialization can't work here, since we need to call the ctor
        // once per obejct instead of per instantiation of this template class
        if (!(m_initialized.test_and_set(std::memory_order_seq_cst))) {
            std::construct_at(p, std::forward<Args>(args)...);
        }
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
        return *get_raw_ptr();
    }

    const T& get() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        return *get_raw_ptr();
    }

    T* ptr() noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        return get_raw_ptr();
    }

    const T* ptr() const noexcept {
#if !defined(COUST_REL)
        if (!is_initialized()) [[unlikely]] {
            std::cerr << std::format(
                "{} not initialized yet\n", type_name<T>());
            DEBUG_BREAK();
        }
#endif
        return get_raw_ptr();
    }

    bool is_initialized() const noexcept {
        return m_initialized.test(std::memory_order_relaxed);
    }

private:
    T* get_raw_ptr() noexcept {
        // std::launder used to preventing undesirable optimization from
        // compiler
        T* const p = std::launder(std::addressof(t));
        return p;
    }

private:
    union {
        alignas(T) char bytes[sizeof(T)];
        T t;
    };
    std::atomic_flag m_initialized = ATOMIC_FLAG_INIT;
};

}  // namespace coust
