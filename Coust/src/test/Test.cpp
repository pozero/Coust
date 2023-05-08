// Tests for codes in header files will be put here, while other codes in
// transilation unit will be tested in place (for convenience)
#if defined(COUST_TEST)
    #include "pch.h"

    #include "utils/Compiler.h"
WARNING_PUSH
DISABLE_ALL_WARNING
    #include "doctest/doctest.h"
WARNING_POP

    #include "utils/AlignedStorage.h"
TEST_CASE("[Coust] [utils] AlingedStorage" * doctest::skip(true)) {
    using namespace coust;
    struct foo {
        int i;
        float f;
        char c;
        int* p;

        foo(int ii, float ff, char cc, int* pp) noexcept
            : i(ii), f(ff), c(cc), p(pp) {}

        foo(const foo& other) noexcept = default;

        foo(foo&& other) noexcept = default;

        ~foo() noexcept {
            if (p)
                *p += 1;
        }

        int emm() const noexcept { return i * 2 + int(f); }
    };
    struct bar {
        foo f1;
        foo f2;
        int i3;
        bar() = delete;
        bar(foo const& ff, foo&& fff, int k) noexcept
            : f1(ff), f2(std::move(fff)), i3(k) {}
    };
    int destruction_count = 0;
    {
        AlignedStorage<bar> bb{};
        [[maybe_unused]] auto constexpr bar_size = sizeof(bb);
        AlignedStorage<bar> bb2{};
        auto const ff = foo{1, 1.0f, 'a', &destruction_count};
        bb.initialize(ff, foo{2, 2.0f, 'b', nullptr}, 3);
        // reinitialization should be ignored
        auto const ff2 = foo{4, 4.0f, 'd', &destruction_count};
        bb.initialize(ff2, foo{5, 5.0, 'e', nullptr}, 6);
        CHECK(bb.get().f1.i == 1);
        CHECK(bb.get().f1.f == 1.0f);
        CHECK(bb.get().f1.c == 'a');
        CHECK(bb.get().f1.p == &destruction_count);
        CHECK(bb.get().f1.emm() == 3);
        CHECK(bb.get().f2.i == 2);
        CHECK(bb.get().f2.f == 2.0f);
        CHECK(bb.get().f2.c == 'b');
        CHECK(bb.get().f2.p == nullptr);
        CHECK(bb.get().f2.emm() == 6);
        CHECK(bb.get().i3 == 3);
        bb2.initialize(ff2, foo{5, 5.0, 'e', nullptr}, 6);
        CHECK(bb2.get().f1.i == 4);
        CHECK(bb2.get().f1.f == 4.0f);
        CHECK(bb2.get().f1.c == 'd');
        CHECK(bb2.get().f1.p == &destruction_count);
        CHECK(bb2.get().f1.emm() == 12);
        CHECK(bb2.get().f2.i == 5);
        CHECK(bb2.get().f2.f == 5.0f);
        CHECK(bb2.get().f2.c == 'e');
        CHECK(bb2.get().f2.p == nullptr);
        CHECK(bb2.get().f2.emm() == 15);
        CHECK(bb2.get().i3 == 6);
    }
    CHECK(destruction_count == 4);
}

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

// pool allocator isn't suitable for stl container, so we don't test it here
    #include "utils/allocators/StlContainer.h"
    #include "utils/allocators/FreeListAllocator.h"
TEST_CASE("[Coust] [utils] [allocators] StdAllocator FreeListAllocator" *
          doctest::skip(true)) {
    using namespace coust;
    memory::MemoryPool mp{memory::byte_512};
    memory::GrowableAllocator<memory::GrowthType::attached, memory::byte_512,
        memory::FreeListAllocator>
        ga{mp};
    using test_alloc = decltype(ga);

    SUBCASE("string") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::string s0{content};
        // create
        memory::string<test_alloc> s1{content, ga};
        CHECK(std::ranges::equal(s0, s1));
        SUBCASE("move ct") {
            memory::string<test_alloc> s2{std::move(s1)};
            CHECK(std::ranges::equal(s0, s2));
        }
        SUBCASE("copy ct") {
            memory::string<test_alloc> s3{s1};
            CHECK(std::ranges::equal(s0, s3));
        }
        SUBCASE("move assign") {
            memory::string<test_alloc> s4 = std::move(s1);
            CHECK(std::ranges::equal(s0, s4));
        }
        SUBCASE("copy assign") {
            memory::string<test_alloc> s5 = s1;
            CHECK(std::ranges::equal(s0, s5));
        }
    }

    SUBCASE("vector<int>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<int> v0{a.begin(), a.end()};
        memory::vector<int, test_alloc> v1{a.begin(), a.end(), ga};
        CHECK(std::ranges::equal(v0, v1));
        SUBCASE("move ct") {
            memory::vector<int, test_alloc> v2{std::move(v1)};
            CHECK(std::ranges::equal(v0, v2));
        }
        SUBCASE("copy ct") {
            memory::vector<int, test_alloc> v3{v1};
            CHECK(std::ranges::equal(v0, v3));
        }
        SUBCASE("move assign") {
            memory::vector<int, test_alloc> v4 = std::move(v1);
            CHECK(std::ranges::equal(v0, v4));
        }
        SUBCASE("copy assign") {
            memory::vector<int, test_alloc> v5 = std::move(v1);
            CHECK(std::ranges::equal(v0, v5));
        }
    }

    SUBCASE("vector<vector<int>>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<std::vector<int>> vv0{
            std::vector<int>{a.rbegin(), a.rend()}
        };
        using container =
            memory::vector_nested<memory::vector<int, test_alloc>, test_alloc>;
        container vv1{ga};
        vv1.emplace_back(a.rbegin(), a.rend());
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<string>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::string> vv0{std::string{content}};
        using container =
            memory::vector_nested<memory::string<test_alloc>, test_alloc>;
        container vv1{ga};
        vv1.emplace_back(content);
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<vector<string>>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::vector<std::string>> vv0{
            std::vector<std::string>{std::string{content}}};
        using container = memory::vector_nested<
            memory::vector_nested<memory::string<test_alloc>, test_alloc>,
            test_alloc>;
        container vv1{ga};
        vv1.emplace_back();
        vv1[0].emplace_back(content);
        CHECK(std::ranges::equal(vv0[0][0], vv1[0][0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0][0], vv2[0][0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0][0], vv3[0][0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0][0], vv4[0][0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0][0], vv5[0][0]));
        }
    }
}

    #include "utils/allocators/HeapAllocator.h"
TEST_CASE("[Coust] [utils] [allocators] StdAllocator HeapAllocator" *
          doctest::skip(true)) {
    using namespace coust;
    memory::HeapAllocator ha{};
    using test_alloc = memory::HeapAllocator;

    SUBCASE("string") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::string s0{content};
        // create
        memory::string<test_alloc> s1{content, ha};
        CHECK(std::ranges::equal(s0, s1));
        SUBCASE("move ct") {
            memory::string<test_alloc> s2{std::move(s1)};
            CHECK(std::ranges::equal(s0, s2));
        }
        SUBCASE("copy ct") {
            memory::string<test_alloc> s3{s1};
            CHECK(std::ranges::equal(s0, s3));
        }
        SUBCASE("move assign") {
            memory::string<test_alloc> s4 = std::move(s1);
            CHECK(std::ranges::equal(s0, s4));
        }
        SUBCASE("copy assign") {
            memory::string<test_alloc> s5 = s1;
            CHECK(std::ranges::equal(s0, s5));
        }
    }

    SUBCASE("vector<int>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<int> v0{a.begin(), a.end()};
        memory::vector<int, test_alloc> v1{a.begin(), a.end(), ha};
        CHECK(std::ranges::equal(v0, v1));
        SUBCASE("move ct") {
            memory::vector<int, test_alloc> v2{std::move(v1)};
            CHECK(std::ranges::equal(v0, v2));
        }
        SUBCASE("copy ct") {
            memory::vector<int, test_alloc> v3{v1};
            CHECK(std::ranges::equal(v0, v3));
        }
        SUBCASE("move assign") {
            memory::vector<int, test_alloc> v4 = std::move(v1);
            CHECK(std::ranges::equal(v0, v4));
        }
        SUBCASE("copy assign") {
            memory::vector<int, test_alloc> v5 = std::move(v1);
            CHECK(std::ranges::equal(v0, v5));
        }
    }

    SUBCASE("vector<vector<int>>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<std::vector<int>> vv0{
            std::vector<int>{a.rbegin(), a.rend()}
        };
        using container =
            memory::vector_nested<memory::vector<int, test_alloc>, test_alloc>;
        container vv1{ha};
        vv1.emplace_back(a.rbegin(), a.rend());
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<string>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::string> vv0{std::string{content}};
        using container =
            memory::vector_nested<memory::string<test_alloc>, test_alloc>;
        container vv1{ha};
        vv1.emplace_back(content);
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<vector<string>>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::vector<std::string>> vv0{
            std::vector<std::string>{std::string{content}}};
        using container = memory::vector_nested<
            memory::vector_nested<memory::string<test_alloc>, test_alloc>,
            test_alloc>;
        container vv1{ha};
        vv1.emplace_back();
        vv1[0].emplace_back(content);
        CHECK(std::ranges::equal(vv0[0][0], vv1[0][0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0][0], vv2[0][0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0][0], vv3[0][0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0][0], vv4[0][0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0][0], vv5[0][0]));
        }
    }
}

    #include "utils/allocators/MonotonicAllocator.h"
TEST_CASE("[Coust] [utils] [allocators] StdAllocator MonotonicAllocator" *
          doctest::skip(true)) {
    using namespace coust;
    memory::MemoryPool mp{memory::byte_512};
    memory::GrowableAllocator<memory::GrowthType::attached, memory::byte_512,
        memory::MonotonicAllocator>
        ga{mp};
    using test_alloc = decltype(ga);

    SUBCASE("string") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::string s0{content};
        // create
        memory::string<test_alloc> s1{content, ga};
        CHECK(std::ranges::equal(s0, s1));
        SUBCASE("move ct") {
            memory::string<test_alloc> s2{std::move(s1)};
            CHECK(std::ranges::equal(s0, s2));
        }
        SUBCASE("copy ct") {
            memory::string<test_alloc> s3{s1};
            CHECK(std::ranges::equal(s0, s3));
        }
        SUBCASE("move assign") {
            memory::string<test_alloc> s4 = std::move(s1);
            CHECK(std::ranges::equal(s0, s4));
        }
        SUBCASE("copy assign") {
            memory::string<test_alloc> s5 = s1;
            CHECK(std::ranges::equal(s0, s5));
        }
    }

    SUBCASE("vector<int>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<int> v0{a.begin(), a.end()};
        memory::vector<int, test_alloc> v1{a.begin(), a.end(), ga};
        CHECK(std::ranges::equal(v0, v1));
        SUBCASE("move ct") {
            memory::vector<int, test_alloc> v2{std::move(v1)};
            CHECK(std::ranges::equal(v0, v2));
        }
        SUBCASE("copy ct") {
            memory::vector<int, test_alloc> v3{v1};
            CHECK(std::ranges::equal(v0, v3));
        }
        SUBCASE("move assign") {
            memory::vector<int, test_alloc> v4 = std::move(v1);
            CHECK(std::ranges::equal(v0, v4));
        }
        SUBCASE("copy assign") {
            memory::vector<int, test_alloc> v5 = std::move(v1);
            CHECK(std::ranges::equal(v0, v5));
        }
    }

    SUBCASE("vector<vector<int>>") {
        std::array constexpr a{2, 4, 6, 8, 10};
        std::vector<std::vector<int>> vv0{
            std::vector<int>{a.rbegin(), a.rend()}
        };
        using container =
            memory::vector_nested<memory::vector<int, test_alloc>, test_alloc>;
        container vv1{ga};
        vv1.emplace_back(a.rbegin(), a.rend());
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<string>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::string> vv0{std::string{content}};
        using container =
            memory::vector_nested<memory::string<test_alloc>, test_alloc>;
        container vv1{ga};
        vv1.emplace_back(content);
        CHECK(std::ranges::equal(vv0[0], vv1[0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0], vv2[0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0], vv3[0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0], vv4[0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0], vv5[0]));
        }
    }

    SUBCASE("vector<vector<string>>") {
        std::string_view content{
            "This is a string that will live on heap instead of stack"};
        std::vector<std::vector<std::string>> vv0{
            std::vector<std::string>{std::string{content}}};
        using container = memory::vector_nested<
            memory::vector_nested<memory::string<test_alloc>, test_alloc>,
            test_alloc>;
        container vv1{ga};
        vv1.emplace_back();
        vv1[0].emplace_back(content);
        CHECK(std::ranges::equal(vv0[0][0], vv1[0][0]));
        SUBCASE("move ct") {
            container vv2{std::move(vv1)};
            CHECK(std::ranges::equal(vv0[0][0], vv2[0][0]));
        }
        SUBCASE("copy ct") {
            container vv3{vv1};
            CHECK(std::ranges::equal(vv0[0][0], vv3[0][0]));
        }
        SUBCASE("move assign") {
            container vv4 = std::move(vv1);
            CHECK(std::ranges::equal(vv0[0][0], vv4[0][0]));
        }
        SUBCASE("copy assign") {
            container vv5 = vv1;
            CHECK(std::ranges::equal(vv0[0][0], vv5[0][0]));
        }
    }
}

    #include "utils/allocators/SmartPtr.h"
TEST_CASE("[Coust] [utils] [allocators] Smart Pointer" * doctest::skip(false)) {
    class Obj {
    public:
        Obj() = delete;

        Obj(int* pb) noexcept : destruct_count(pb) {}

        ~Obj() { (*destruct_count)++; }

    private:
        int* destruct_count;
    };

    using namespace coust;
    memory::MemoryPool mp{memory::byte_512};
    memory::GrowableAllocator<memory::GrowthType::attached, memory::byte_512,
        memory::FreeListAllocator>
        ga{mp};

    int count = 0;
    SUBCASE("unique pointer") {
        {
            auto unique = memory::allocate_unique<Obj>(ga, &count);
            Obj* p1 = unique.release();
            unique = memory::allocate_unique<Obj>(ga, &count);
            p1->~Obj();
            ga.deallocate(p1, sizeof(Obj));
        }
        CHECK(count == 2);
    }

    SUBCASE("shared pointer") {
        {
            std::shared_ptr<Obj> shared =
                memory::allocate_shared<Obj>(ga, &count);
            std::shared_ptr<Obj> shared_1{shared};
            std::shared_ptr<Obj> shared_2{shared};
        }
        CHECK(count == 1);
    }
}

#endif
