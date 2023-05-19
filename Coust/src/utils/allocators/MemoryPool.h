#pragma once

#include "utils/allocators/Allocator.h"
#include "utils/allocators/Area.h"

#include <deque>
#include <initializer_list>

namespace coust {
namespace memory {

class MemoryPool {
public:
    MemoryPool() = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

public:
    MemoryPool(std::initializer_list<Size> const& all_sizes,
        size_t alignment = DEFAULT_ALIGNMENT) noexcept;

    Area allocate_area(size_t size, size_t alignment) noexcept;

    void deallocate_area(Area&& free_area) noexcept;

private:
    static size_t constexpr RAW_AREA_SIZE_MULTIPLIER = 1u;

private:
    // raw areas control the actual allocation and deallocation (system call)
    std::deque<Area> m_raw_areas;
    // to reduce fragmentation and allocation call, we split the raw areas into
    // smaller scoped areas and provide them to allocators instead of raw areas
    std::deque<std::deque<Area>> m_split_areas;
    std::deque<Size> m_sizes;
    [[maybe_unused]] size_t const m_alignement;

private:
    size_t find_pool(size_t size) const noexcept;
};

}  // namespace memory
}  // namespace coust
