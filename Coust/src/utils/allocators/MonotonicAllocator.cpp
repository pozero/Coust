#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/MonotonicAllocator.h"

namespace coust {
namespace memory {

MonotonicAllocator::MonotonicAllocator(void* begin, void* end) noexcept
    : m_cur_begin(begin), m_cur_end(end) {
}

void* MonotonicAllocator::allocate(size_t size, size_t alignment) noexcept {
    void* const ret_ptr = ptr_math::align(m_cur_begin, alignment);
    void* const next_begin = ptr_math::add(ret_ptr, size);
    if (next_begin > m_cur_end)
        return nullptr;
    m_cur_begin = next_begin;
    return ret_ptr;
}

void MonotonicAllocator::deallocate(
    [[maybe_unused]] void*, [[maybe_unused]] size_t) noexcept {
}

void MonotonicAllocator::grow(void* p, size_t size) noexcept {
    m_cur_begin = p;
    m_cur_end = ptr_math::add(p, size);
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

TEST_CASE(
    "[Coust] [utils] [allocators] MonotonicAllocator" * doctest::skip(true)) {
    using namespace coust::memory;
    size_t constexpr experiment_cnt = 50;
    MemoryPool mp{byte_32, byte_64};

    SUBCASE("Manually growth") {
        GrowthPolicy<GrowthType::attached, byte_32> gp{mp};
        std::array<char, byte_32> stack_area{};
        MonotonicAllocator ma{stack_area.data(),
            ptr_math::add(stack_area.data(), stack_area.size())};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ma.allocate(sizeof(float), alignof(float));
            if (fp == nullptr) {
                auto const [ptr, size] = gp.do_growth(alignof(float));
                ma.grow(ptr, size);
                fp = (float*) ma.allocate(sizeof(float), alignof(float));
            }
            *fp = (float) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (attached)") {
        GrowableAllocator<GrowthType::attached, byte_64, MonotonicAllocator> ga{
            mp};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (scope)") {
        size_t constexpr area_size = experiment_cnt * sizeof(float);
        std::array<char, area_size> stack_area{};
        GrowableAllocator<GrowthType::scope, byte_32, MonotonicAllocator> ga{
            stack_area};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (heap)") {
        GrowableAllocator<GrowthType::heap, byte_32, MonotonicAllocator> ga{};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }
}

#endif
