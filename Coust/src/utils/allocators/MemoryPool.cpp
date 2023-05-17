#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/MemoryPool.h"
#include "utils/Assert.h"

namespace coust {
namespace memory {

MemoryPool::MemoryPool(
    std::initializer_list<Size> const& all_sizes, size_t alignment) noexcept
    : m_sizes(all_sizes), m_alignement(alignment) {
    std::ranges::sort(m_sizes);
    m_split_areas.resize(all_sizes.size());
}

Area MemoryPool::allocate_area(size_t size, size_t alignment) noexcept {
    COUST_ASSERT(alignment <= m_alignement,
        "Memory pool can't meet the alignment requirement, the requested "
        "alignment is {}",
        alignment);
    size_t const pool_idx = find_pool(size);
    size_t const actual_allocation_size = m_sizes[pool_idx];
    std::deque<Area>& pool = m_split_areas[pool_idx];
    if (!pool.empty()) {
        Area area = std::move(pool.front());
        pool.pop_front();
        return area;
    } else {
        auto const& raw_area = m_raw_areas.emplace_back(
            m_sizes.back() * RAW_AREA_SIZE_MULTIPLIER, alignment);
        auto split_areas = Area::split_areas(raw_area, actual_allocation_size);
        Area ret = std::move(split_areas.back());
        split_areas.pop_back();
        std::ranges::move(split_areas, std::back_inserter(pool));
        return ret;
    }
}

void MemoryPool::deallocate_area(Area&& free_area) noexcept {
    size_t const free_size = free_area.size();
    size_t const pool_idx = find_pool(free_size);
    std::deque<Area>& pool = m_split_areas[pool_idx];
    pool.push_front(std::move(free_area));
}

size_t MemoryPool::find_pool(size_t size) const noexcept {
    auto iter = std::ranges::lower_bound(m_sizes, size);
    COUST_ASSERT(iter != m_sizes.end(),
        "Size {} exceeds the biggest size of this pool: {}", size,
        to_string_view(m_sizes.back()));
    return (size_t) std::distance(m_sizes.begin(), iter);
}

}  // namespace memory
}  // namespace coust
