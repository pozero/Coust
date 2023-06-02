#pragma once

#include "utils/Assert.h"
#include "utils/allocators/Allocator.h"

namespace coust {
namespace memory {

template <typename T, detail::Allocator Alloc>
class StdAllocator {
public:
public:
    using allocator_type = Alloc;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal =
        std::bool_constant<!allocator_type::stateful::value>;
    using reference = T&;
    using const_reference = const T&;

    template <typename U>
    struct rebind {
        using other = StdAllocator<U, Alloc>;
    };

public:
    // std::scoped_allocator requires a default constructor
    explicit StdAllocator() noexcept = default;

    StdAllocator(allocator_type& a) noexcept : m_alloc_ptr(&a) {}

    template <typename U>
    StdAllocator(StdAllocator<U, Alloc> const& other) noexcept
        : m_alloc_ptr(other.m_alloc_ptr) {}

    StdAllocator(StdAllocator const& other) noexcept
        : m_alloc_ptr(other.m_alloc_ptr) {}

    template <typename U>
    StdAllocator& operator=(StdAllocator<U, Alloc> const& other) noexcept {
        m_alloc_ptr = other.m_alloc_ptr;
        return *this;
    }

    StdAllocator& operator=(StdAllocator const& other) noexcept {
        m_alloc_ptr = other.m_alloc_ptr;
        return *this;
    }

    template <typename U>
    StdAllocator(StdAllocator<U, Alloc>&& other) noexcept
        : m_alloc_ptr(other.m_alloc_ptr) {}

    StdAllocator(StdAllocator&& other) noexcept
        : m_alloc_ptr(other.m_alloc_ptr) {}

    template <typename U>
    StdAllocator& operator=(StdAllocator<U, Alloc>&& other) noexcept {
        m_alloc_ptr = other.m_alloc_ptr;
        return *this;
    }

    StdAllocator& operator=(StdAllocator&& other) noexcept {
        m_alloc_ptr = other.m_alloc_ptr;
        return *this;
    }

public:
    T* allocate(std::size_t n) noexcept {
        COUST_ASSERT(m_alloc_ptr, "You forgot to assign alloctar!");
        return (T*) m_alloc_ptr->allocate(n * sizeof(T), alignof(T));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        COUST_ASSERT(m_alloc_ptr, "You forgot to assign alloctar!");
        m_alloc_ptr->deallocate(p, n * sizeof(T));
    }

    StdAllocator<T, Alloc> select_on_container_copy_construction()
        const noexcept {
        return StdAllocator<T, Alloc>{*m_alloc_ptr};
    }

    template <typename U, detail::Allocator AAlloc>
    bool operator==(StdAllocator<U, AAlloc> const& rhs) const noexcept {
        return std::addressof(m_alloc_ptr) == std::addressof(rhs.m_alloc_ptr);
    }

    template <typename U, detail::Allocator AAlloc>
    bool operator!=(StdAllocator<U, AAlloc> const& rhs) const noexcept {
        return !operator==(rhs);
    }

private:
    template <typename U, detail::Allocator AAlloc>
    friend class StdAllocator;

    Alloc* m_alloc_ptr = nullptr;
};

}  // namespace memory
}  // namespace coust
