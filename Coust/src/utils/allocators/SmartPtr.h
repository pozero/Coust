#pragma once

#include "utils/allocators/Allocator.h"
#include "utils/allocators/StlAdaptor.h"

namespace coust {
namespace memory {
namespace detail {

template <typename T, Allocator Alloc>
class AllocatorDeleter {
public:
    using allocator_type = Alloc;
    using value_type = T;

public:
    AllocatorDeleter() noexcept = default;

    AllocatorDeleter(Alloc& allocator) noexcept : m_allocator_ptr(&allocator) {}

    void operator()(value_type* ptr) noexcept {
        static_assert(!std::is_abstract_v<value_type>,
            "Allocator deleter doesn't support abstract class");
        ptr->~value_type();
        m_allocator_ptr->deallocate(ptr, sizeof(value_type));
    }

    Alloc& get_allocator() const noexcept { return *m_allocator_ptr; }

private:
    Alloc* m_allocator_ptr = nullptr;
};

}  // namespace detail

template <typename T, detail::Allocator Alloc>
using unique_ptr = std::unique_ptr<T, detail::AllocatorDeleter<T, Alloc>>;

// no actual meaning, just declared for convenience
template <typename T>
using shared_ptr = std::shared_ptr<T>;

template <typename T, detail::Allocator Alloc, typename... Args>
auto allocate_unique(Alloc& alloc, Args&&... args)
    -> std::unique_ptr<T, detail::AllocatorDeleter<T, Alloc>> {
    T* raw_ptr = (T*) alloc.allocate(sizeof(T), alignof(T));
    std::construct_at(raw_ptr, std::forward<Args>(args)...);
    return {raw_ptr, {alloc}};
}

template <typename T, detail::Allocator Alloc, typename... Args>
auto allocate_shared(Alloc& alloc, Args&&... args) -> std::shared_ptr<T> {
    StdAllocator<T, Alloc> std_alloc{alloc};
    return std::allocate_shared<T>(std_alloc, std::forward<Args>(args)...);
}

}  // namespace memory
}  // namespace coust
