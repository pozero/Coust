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
    std::deque<std::deque<Area>> m_areas;
    std::deque<Size> m_sizes;
    [[maybe_unused]] size_t const m_alignement;

private:
    size_t find_pool(size_t size) const noexcept;
};

}  // namespace memory
}  // namespace coust
