#pragma once

#include "utils/allocators/Allocator.h"
#include "utils/allocators/Area.h"

namespace coust {
namespace memory {

class PoolAllocator {
public:
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator(PoolAllocator const&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;
    PoolAllocator operator=(PoolAllocator const&) = delete;

public:
    using stateful = std::true_type;

public:
    PoolAllocator(size_t node_size) noexcept;

    PoolAllocator(void* begin, void* end, size_t node_size) noexcept;

    void* allocate(size_t size, size_t) noexcept;

    void deallocate(void* p, size_t) noexcept;

    void grow(void* p, size_t size) noexcept;

private:
    struct Node {
        Node* next = nullptr;
    };

    Node* m_first = nullptr;
    void* m_unindexed_begin = nullptr;
    void* m_unindexed_end = nullptr;
    size_t const m_node_size;
};

static_assert(detail::GrowableAllocator<PoolAllocator>, "");

}  // namespace memory
}  // namespace coust
