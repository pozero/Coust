#include "pch.h"

#include "test/Test.h"

#include "utils/allocators/GrowthPolicy.h"
TEST_CASE("[Coust] [utils] [allocators] GrowthPolicy" * doctest::skip(true)) {
    using namespace coust::memory;
    SUBCASE("Scoped growth policy") {
        size_t constexpr area_count = 5;
        {
            Size constexpr area_size = byte_32;
            {
                std::array<char, area_count * area_size> stack_area;
                GrowthPolicy<GrowthType::scope, area_size> growth{stack_area};
                for ([[maybe_unused]] auto i :
                    std::views::iota(0u, area_count)) {
                    auto [ptr, size] = growth.do_growth(DEFAULT_ALIGNMENT);
                    CHECK(size == area_size);
                }
            }
            {
                auto heap_area =
                    std::make_unique<char[]>(area_count * area_size);
                GrowthPolicy<GrowthType::scope, area_size> growth{
                    heap_area.get(),
                    ptr_math::add(heap_area.get(), area_count * area_size)};
                for ([[maybe_unused]] auto i :
                    std::views::iota(0u, area_count)) {
                    auto [ptr, size] = growth.do_growth(DEFAULT_ALIGNMENT);
                    CHECK(size == area_size);
                }
            }
        }
        {
            Size constexpr area_size = byte_64;
            {
                std::array<char, area_count * area_size> stack_area;
                GrowthPolicy<GrowthType::scope, area_size> growth{stack_area};
                for ([[maybe_unused]] auto i :
                    std::views::iota(0u, area_count)) {
                    auto [ptr, size] = growth.do_growth(DEFAULT_ALIGNMENT);
                    CHECK(size == area_size);
                }
            }
            {
                auto heap_area =
                    std::make_unique<char[]>(area_count * area_size);
                GrowthPolicy<GrowthType::scope, area_size> growth{
                    heap_area.get(),
                    ptr_math::add(heap_area.get(), area_count * area_size)};
                for ([[maybe_unused]] auto i :
                    std::views::iota(0u, area_count)) {
                    auto [ptr, size] = growth.do_growth(DEFAULT_ALIGNMENT);
                    CHECK(size == area_size);
                }
            }
        }
    }

    SUBCASE("Attached growth policy") {
        Size constexpr area_size = byte_64;
        MemoryPool mp{byte_32, byte_64, byte_128};
        void* p1 = nullptr;
        void* p2 = nullptr;
        void* p3 = nullptr;
        void* p4 = nullptr;
        {
            GrowthPolicy<GrowthType::attached, area_size> gp{mp};
            auto [p1_, s1] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p2_, s2] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p3_, s3] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p4_, s4] = gp.do_growth(DEFAULT_ALIGNMENT);
            p1 = p1_;
            p2 = p2_;
            p3 = p3_;
            p4 = p4_;
            CHECK(s1 == area_size);
            CHECK(s2 == area_size);
            CHECK(s3 == area_size);
            CHECK(s4 == area_size);
        }
        {
            GrowthPolicy<GrowthType::attached, area_size> gp{mp};
            auto [p1_, s1] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p2_, s2] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p3_, s3] = gp.do_growth(DEFAULT_ALIGNMENT);
            auto [p4_, s4] = gp.do_growth(DEFAULT_ALIGNMENT);
            CHECK(p1_ == p1);
            CHECK(p2_ == p2);
            CHECK(p3_ == p3);
            CHECK(p4_ == p4);
        }
    }

    SUBCASE("Heap growth policy") {
        Size constexpr area_size = byte_128;
        GrowthPolicy<GrowthType::heap, area_size> gp{};
        for (int i = 0; i < 12; ++i) {
            auto [ptr, size] = gp.do_growth(DEFAULT_ALIGNMENT);
            CHECK(size == area_size);
        }
    }
}
