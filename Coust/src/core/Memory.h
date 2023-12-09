#pragma once

#include "utils/allocators/MemoryPool.h"
#include "utils/allocators/GrowthPolicy.h"
#include "utils/allocators/FreeListAllocator.h"
#include "utils/allocators/HeapAllocator.h"
#include "utils/allocators/PoolAllocator.h"

namespace coust {
namespace memory {

// ref:
// https://gdcvault.com/play/1023309/Building-a-Low-Fragmentation-Memory

using RobustAlloc = HeapAllocator;

Size constexpr small_growth_factor = kbyte_1;
Size constexpr medium_growth_factor = kbyte_5;
Size constexpr large_growth_factor = kbyte_50;

size_t constexpr global_memory_pool_alignment = DEFAULT_ALIGNMENT;

using HomoAlloc_S =
    GrowableAllocator<GrowthType::attached, small_growth_factor, PoolAllocator>;

using GeneralAlloc_M = GrowableAllocator<GrowthType::attached,
    medium_growth_factor, FreeListAllocator>;

using GeneralAlloc_L = GrowableAllocator<GrowthType::attached,
    large_growth_factor, FreeListAllocator>;

MemoryPool& get_global_memory_pool() noexcept;

// techinically, this class should be a abstract template like
// `Aggreagator<Raw_Alloc1, Size, Raw_Alloc2>`, and we can compose them like
// `Aggregator<Aggregator<...>, Size, Aggregator<...>>`, but that would
// introduce too much template instantiation. since we just use it once (or
// twice in render backend?), the fallback logic is directly written here
class AggregateAllocator {
public:
    AggregateAllocator() = delete;
    AggregateAllocator(AggregateAllocator&&) = delete;
    AggregateAllocator(AggregateAllocator const&) = delete;
    AggregateAllocator& operator=(AggregateAllocator&&) = delete;
    AggregateAllocator& operator=(AggregateAllocator const&) = delete;

public:
    using stateful = std::true_type;

public:
    AggregateAllocator(MemoryPool& pool) noexcept;

    void* allocate(size_t size, size_t alignment) noexcept;

    void deallocate(void* p, size_t size) noexcept;

    template <typename T, typename... Args>
    T* construct(Args&&... args) noexcept
        requires(std::is_constructible_v<T, Args...>)
    {
        T* ptr = (T*) allocate(sizeof(T), alignof(T));
        std::construct_at(ptr, std::forward<Args>(args)...);
        return ptr;
    }

    template <typename T>
    void destruct(T* ptr) noexcept {
        ptr->~T();
        deallocate(ptr, sizeof(T));
    }

private:
    HomoAlloc_S m_8byte_alloc;
    HomoAlloc_S m_16byte_alloc;
    HomoAlloc_S m_32byte_alloc;
    HomoAlloc_S m_64byte_alloc;
    HomoAlloc_S m_128byte_alloc;
    GeneralAlloc_M m_upto_5kbyte_alloc;
    GeneralAlloc_L m_upto_50kbyte_alloc;
    HeapAllocator m_gaigantic_alloc;
};

static_assert(detail::Allocator<AggregateAllocator>);

}  // namespace memory

using DefaultAlloc = memory::AggregateAllocator;

DefaultAlloc& get_default_alloc() noexcept;

}  // namespace coust
