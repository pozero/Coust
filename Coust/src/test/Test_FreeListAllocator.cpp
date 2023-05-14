#include "pch.h"

#include "test/Test.h"

#include "utils/allocators/MemoryPool.h"
#include "utils/allocators/GrowthPolicy.h"
#include "utils/allocators/FreeListAllocator.h"

TEST_CASE(
    "[Coust] [utils] [allocators] FreeListAllocator" * doctest::skip(true)) {
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
        size_t constexpr area_size = 3 * max_ele_cnt * sizeof(Obj);
        size_t constexpr experiment_cnt = 10;
        Area area{area_size, alignof(Obj)};
        FreeListAllocator fa{area.begin(), area.end()};
        std::vector<Obj*> all_objs{};
        all_objs.reserve(max_ele_cnt);
        std::array<size_t, experiment_cnt> experiment_sizes{};
        for (size_t i = 0; i < experiment_cnt; ++i) {
            size_t constexpr inc = max_ele_cnt / experiment_cnt;
            experiment_sizes[i] = (i + 1) * inc;
        }
        std::random_device rd;
        std::mt19937 gen{rd()};
        std::ranges::shuffle(experiment_sizes, gen);
        for (auto s : experiment_sizes) {
            for (size_t i = 0; i < s; ++i) {
                Obj* op = (Obj*) fa.allocate(sizeof(Obj), alignof(Obj));
                std::construct_at<Obj>(op, (int) i);
                all_objs.push_back(op);
                fa.is_malfunctioning();
            }
            for (auto p : all_objs) {
                CHECK(2 * p->i1 == p->i2);
                CHECK(3 * p->i1 == p->i3);
                fa.deallocate(p, sizeof(Obj));
                fa.is_malfunctioning();
            }
            all_objs.clear();
        }
    }

    SUBCASE("Manually growth") {
        MemoryPool mp{byte_32, byte_64};
        size_t constexpr experiment_cnt = 50;
        GrowthPolicy<GrowthType::attached, byte_32> gp{mp};
        std::array<char, byte_32> stack_area{};
        FreeListAllocator ma{stack_area.data(),
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
            ma.is_malfunctioning();
            *fp = (float) i;
            fps.push_back(fp);
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (attached)") {
        MemoryPool mp{byte_32, byte_64};
        size_t constexpr experiment_cnt = 50;
        GrowableAllocator<GrowthType::attached, byte_64, FreeListAllocator> ga{
            mp};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
            ga.get_raw_allocator().is_malfunctioning();
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (scope)") {
        size_t constexpr experiment_cnt = 50;
        size_t constexpr area_size = 10 * experiment_cnt * sizeof(float);
        std::array<char, area_size> stack_area{};
        GrowableAllocator<GrowthType::scope, byte_32, FreeListAllocator> ga{
            stack_area};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
            ga.get_raw_allocator().is_malfunctioning();
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }

    SUBCASE("Auto growth (heap)") {
        MemoryPool mp{byte_32, byte_64};
        size_t constexpr experiment_cnt = 50;
        GrowableAllocator<GrowthType::heap, byte_32, FreeListAllocator> ga{};
        std::vector<float*> fps{};
        fps.reserve(experiment_cnt);
        for (size_t i = 0; i < experiment_cnt; ++i) {
            float* fp = (float*) ga.allocate(sizeof(float), alignof(float));
            *fp = (float) i;
            fps.push_back(fp);
            ga.get_raw_allocator().is_malfunctioning();
        }
        for (size_t i = 0; i < experiment_cnt; ++i) {
            CHECK(*fps[i] == (float) i);
        }
    }
}
