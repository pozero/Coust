#include "pch.h"

#include "utils/Compiler.h"
#include "core/Memory.h"

namespace coust {
namespace memory {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wexit-time-destructors")
MemoryPool& get_global_memory_pool() noexcept {
    static MemoryPool s_global_memory_pool{
        {small_growth_factor, medium_growth_factor, large_growth_factor},
        global_memory_pool_alignment
    };
    return s_global_memory_pool;
}
WARNING_POP

AggregateAllocator::AggregateAllocator(MemoryPool& pool) noexcept
    : m_8byte_alloc(pool, byte_8),
      m_16byte_alloc(pool, byte_16),
      m_32byte_alloc(pool, byte_32),
      m_64byte_alloc(pool, byte_64),
      m_128byte_alloc(pool, byte_128),
      m_upto_5kbyte_alloc(pool),
      m_upto_50kbyte_alloc(pool) {
}

void* AggregateAllocator::allocate(size_t size, size_t alignment) noexcept {
    COUST_ASSERT(size != 0, "Allocation memory with size 0 is problematic");
    if (size <= byte_8) {
        return m_8byte_alloc.allocate(size, alignment);
    } else if (size <= byte_16) {
        return m_16byte_alloc.allocate(size, alignment);
    } else if (size <= byte_32) {
        return m_32byte_alloc.allocate(size, alignment);
    } else if (size <= byte_64) {
        return m_64byte_alloc.allocate(size, alignment);
    } else if (size <= byte_128) {
        return m_128byte_alloc.allocate(size, alignment);
    } else if (size <= kbyte_5) {
        return m_upto_5kbyte_alloc.allocate(size, alignment);
    } else if (size <= kbyte_50) {
        return m_upto_50kbyte_alloc.allocate(size, alignment);
    } else {
        COUST_INFO("Giant Allocation: size {}, alignment {}", size, alignment);
        return m_gaigantic_alloc.allocate(size, alignment);
    }
}

void AggregateAllocator::deallocate(void* p, size_t size) noexcept {
    if (p == nullptr)
        return;
    if (size <= byte_8) {
        return m_8byte_alloc.deallocate(p, size);
    } else if (size <= byte_16) {
        return m_16byte_alloc.deallocate(p, size);
    } else if (size <= byte_32) {
        return m_32byte_alloc.deallocate(p, size);
    } else if (size <= byte_64) {
        return m_64byte_alloc.deallocate(p, size);
    } else if (size <= byte_128) {
        return m_128byte_alloc.deallocate(p, size);
    } else if (size <= kbyte_5) {
        return m_upto_5kbyte_alloc.deallocate(p, size);
    } else if (size <= kbyte_50) {
        return m_upto_50kbyte_alloc.deallocate(p, size);
    } else {
        return m_gaigantic_alloc.deallocate(p, size);
    }
}

}  // namespace memory

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wexit-time-destructors")
DefaultAlloc& get_default_alloc() noexcept {
    static DefaultAlloc s_default_allocator{memory::get_global_memory_pool()};
    return s_default_allocator;
}
WARNING_POP

}  // namespace coust
