#include "pch.h"

#include "utils/Compiler.h"
#include "utils/Assert.h"
#include "utils/allocators/PoolAllocator.h"

namespace coust {
namespace memory {

PoolAllocator::PoolAllocator(size_t node_size) noexcept
    : m_node_size(node_size) {
    COUST_ASSERT(node_size >= sizeof(Node),
        "node_size {} is too small to maintain internal structure of pool "
        "allocator, and it should be at least {}",
        node_size, sizeof(Node));
}

PoolAllocator::PoolAllocator(void* begin, void* end, size_t node_size) noexcept
    : m_unindexed_begin(begin), m_unindexed_end(end), m_node_size(node_size) {
    COUST_ASSERT(node_size >= sizeof(Node),
        "node_size {} is too small to maintain internal structure of pool "
        "allocator, and it should be at least {}",
        node_size, sizeof(Node));
}

void* PoolAllocator::allocate(
    [[maybe_unused]] size_t size, [[maybe_unused]] size_t) noexcept {
    COUST_ASSERT(size <= m_node_size,
        "size requirement exceeds node size of pool allocator, requirement: "
        "{}, node_size: {}",
        size, m_node_size);
    if (m_first) {
        void* const ret = m_first;
        m_first = m_first->next;
        return ret;
    }
    void* const next_begin = ptr_math::add(m_unindexed_begin, m_node_size);
    if (next_begin <= m_unindexed_end) {
        void* const ret = m_unindexed_begin;
        m_unindexed_begin = next_begin;
        return ret;
    } else {
        return nullptr;
    }
}

void PoolAllocator::deallocate(void* p, [[maybe_unused]] size_t) noexcept {
    Node* RESTRICT const node = (Node*) p;
    node->next = m_first;
    m_first = node;
}

void PoolAllocator::grow(void* p, size_t size) noexcept {
    COUST_ASSERT(
        ptr_math::sub(m_unindexed_end, m_unindexed_begin) < m_node_size,
        "Can't grow pool allocator while there's still unindexed free space, "
        "node_size: {}, unindexed: {}, {}",
        m_node_size, m_unindexed_begin, m_unindexed_end);
    m_unindexed_begin = p;
    m_unindexed_end = ptr_math::add(m_unindexed_begin, size);
}

}  // namespace memory
}  // namespace coust
