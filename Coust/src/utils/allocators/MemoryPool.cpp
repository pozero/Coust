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
    m_areas.resize(all_sizes.size());
}

Area MemoryPool::allocate_area(size_t size, size_t alignment) noexcept {
    COUST_ASSERT(alignment <= m_alignement,
        "Memory pool can't meet the alignment requirement, the requested "
        "alignment is {}",
        alignment);
    size_t const pool_idx = find_pool(size);
    std::deque<Area>& pool = m_areas[pool_idx];
    if (!pool.empty()) {
        Area area = std::move(pool.front());
        pool.pop_front();
        return area;
    } else {
        return Area{m_sizes[pool_idx], alignment};
    }
}

void MemoryPool::deallocate_area(Area&& free_area) noexcept {
    size_t const free_size = free_area.size();
    size_t const pool_idx = find_pool(free_size);
    std::deque<Area>& pool = m_areas[pool_idx];
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

#if defined(COUST_TEST)

WARNING_PUSH
DISABLE_ALL_WARNING
    #include "doctest/doctest.h"
WARNING_POP

TEST_CASE("[Coust] [utils] [allocators] MemoryPool" * doctest::skip(true)) {
    using namespace coust::memory;

    MemoryPool mp{byte_32, byte_64, byte_128};

    SUBCASE("32 bytes area") {
        void* p1 = nullptr;
        void* p2 = nullptr;
        void* p3 = nullptr;
        void* p4 = nullptr;
        {  // all areas should be in size 32
            Area area1{mp.allocate_area(1, DEFAULT_ALIGNMENT)};
            Area area2{mp.allocate_area(10, DEFAULT_ALIGNMENT)};
            Area area3{mp.allocate_area(20, DEFAULT_ALIGNMENT)};
            Area area4{mp.allocate_area(32, DEFAULT_ALIGNMENT)};
            p1 = area1.begin();
            p2 = area2.begin();
            p3 = area3.begin();
            p4 = area4.begin();
            CHECK(area1.size() == 32);
            CHECK(area2.size() == 32);
            CHECK(area3.size() == 32);
            CHECK(area4.size() == 32);
            mp.deallocate_area(std::move(area1));
            mp.deallocate_area(std::move(area2));
            mp.deallocate_area(std::move(area3));
            mp.deallocate_area(std::move(area4));
        }
        {  // areas should be freed in FIFO order
            Area area4{mp.allocate_area(20, DEFAULT_ALIGNMENT)};
            Area area3{mp.allocate_area(13, DEFAULT_ALIGNMENT)};
            Area area2{mp.allocate_area(25, DEFAULT_ALIGNMENT)};
            Area area1{mp.allocate_area(32, DEFAULT_ALIGNMENT)};
            CHECK(area1.begin() == p1);
            CHECK(area2.begin() == p2);
            CHECK(area3.begin() == p3);
            CHECK(area4.begin() == p4);
        }
    }

    SUBCASE("128 bytes area") {
        void* p1 = nullptr;
        void* p2 = nullptr;
        void* p3 = nullptr;
        void* p4 = nullptr;
        {  // all areas should be in size 128
            Area area1{mp.allocate_area(65, DEFAULT_ALIGNMENT)};
            Area area2{mp.allocate_area(75, DEFAULT_ALIGNMENT)};
            Area area3{mp.allocate_area(100, DEFAULT_ALIGNMENT)};
            Area area4{mp.allocate_area(128, DEFAULT_ALIGNMENT)};
            p1 = area1.begin();
            p2 = area2.begin();
            p3 = area3.begin();
            p4 = area4.begin();
            CHECK(area1.size() == 128);
            CHECK(area2.size() == 128);
            CHECK(area3.size() == 128);
            CHECK(area4.size() == 128);
            mp.deallocate_area(std::move(area1));
            mp.deallocate_area(std::move(area2));
            mp.deallocate_area(std::move(area3));
            mp.deallocate_area(std::move(area4));
        }
        {  // areas should be freed in FIFO order
            Area area4{mp.allocate_area(108, DEFAULT_ALIGNMENT)};
            Area area3{mp.allocate_area(110, DEFAULT_ALIGNMENT)};
            Area area2{mp.allocate_area(65, DEFAULT_ALIGNMENT)};
            Area area1{mp.allocate_area(128, DEFAULT_ALIGNMENT)};
            CHECK(area1.begin() == p1);
            CHECK(area2.begin() == p2);
            CHECK(area3.begin() == p3);
            CHECK(area4.begin() == p4);
        }
    }

    SUBCASE("Memory inside Area should be persistent") {
        float constexpr f = 3.1515f;
        int constexpr i = 23;
        char constexpr c = 'e';
        {
            Area area1{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
            Area area2{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
            Area area3{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
            *((float*) area1.begin()) = f;
            *((int*) area2.begin()) = i;
            *((char*) area3.begin()) = c;
            mp.deallocate_area(std::move(area3));
            mp.deallocate_area(std::move(area2));
            mp.deallocate_area(std::move(area1));
        }
        Area area1{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
        Area area2{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
        Area area3{mp.allocate_area(64, DEFAULT_ALIGNMENT)};
        CHECK((*(float*) area1.begin()) == f);
        CHECK((*(int*) area2.begin()) == i);
        CHECK((*(char*) area3.begin()) == c);
    }
}

#endif
