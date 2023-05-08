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

#if defined(COUST_TEST)

WARNING_PUSH
DISABLE_ALL_WARNING
    #include "doctest/doctest.h"
WARNING_POP
    #include "utils/allocators/MemoryPool.h"
    #include "utils/allocators/GrowthPolicy.h"

TEST_CASE("[coust] [utils] [PoolAllocator]" * doctest::skip(true)) {
    using namespace coust::memory;

    SUBCASE("Test memory leak") {
        struct Obj {
            int i1;
            int i2;
            int i3;
            Obj(int i_) noexcept : i1(i_), i2(2 * i1), i3(3 * i1) {}
            // bool is_intact() const noexcept { return i2 == 2 * i1 && i3 == 3
            // * i1; }
        };
        size_t constexpr max_ele_cnt = 50;
        size_t constexpr area_size = max_ele_cnt * sizeof(Obj);
        size_t constexpr experiment_cnt = 10;
        Area area{area_size, alignof(Obj)};
        PoolAllocator pa{area.begin(), area.end(), sizeof(Obj)};
        std::vector<Obj*> all_objs{};
        all_objs.reserve(max_ele_cnt);
        std::array<size_t, experiment_cnt> experiment_sizes{};
        for (size_t i = 1; i <= experiment_cnt; ++i) {
            size_t constexpr inc = max_ele_cnt / experiment_cnt;
            experiment_sizes[i - 1] = i * inc;
        }
        std::random_device rd;
        std::mt19937 gen{rd()};
        std::ranges::shuffle(experiment_sizes, gen);
        for (auto s : experiment_sizes) {
            for (size_t i = 0; i < s; ++i) {
                Obj* op = (Obj*) pa.allocate(sizeof(Obj), alignof(Obj));
                std::construct_at<Obj>(op, (int) i);
                all_objs.push_back(op);
            }
            for (auto p : all_objs) {
                CHECK(2 * p->i1 == p->i2);
                CHECK(3 * p->i1 == p->i3);
                pa.deallocate(p, sizeof(Obj));
            }
            all_objs.clear();
        }
    }

    size_t constexpr experiment_cnt = 50;
    MemoryPool mp{byte_32, byte_64};

    SUBCASE("Manually growth") {
        GrowthPolicy<GrowthType::attached, byte_32> gp{mp};
        std::array<char, byte_32> stack_area{};
        PoolAllocator ma{stack_area.data(),
            ptr_math::add(stack_area.data(), stack_area.size()),
            sizeof(double)};
        std::vector<double*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            double* fp = (double*) ma.allocate(sizeof(double), alignof(double));
            if (fp == nullptr) {
                auto const [ptr, size] = gp.do_growth(alignof(double));
                ma.grow(ptr, size);
                fp = (double*) ma.allocate(sizeof(double), alignof(double));
            }
            *fp = (double) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (double) i);
        }
    }

    SUBCASE("Auto growth (attached)") {
        GrowableAllocator<GrowthType::attached, byte_64, PoolAllocator> pa{
            mp, sizeof(double)};
        std::vector<double*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            double* fp = (double*) pa.allocate(sizeof(double), alignof(double));
            *fp = (double) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (double) i);
        }
    }

    SUBCASE("Auto growth (scope)") {
        size_t constexpr area_size = experiment_cnt * sizeof(double);
        std::array<char, area_size> stack_area{};
        GrowableAllocator<GrowthType::scope, byte_32, PoolAllocator> pa{
            stack_area, sizeof(double)};
        std::vector<double*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            double* fp = (double*) pa.allocate(sizeof(double), alignof(double));
            *fp = (double) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (double) i);
        }
    }

    SUBCASE("Auto growth (heap)") {
        GrowableAllocator<GrowthType::heap, byte_32, PoolAllocator> pa{
            sizeof(double)};
        std::vector<double*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            double* fp = (double*) pa.allocate(sizeof(double), alignof(double));
            *fp = (double) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (double) i);
        }
    }
}

#endif
