#include "pch.h"

#include "test/Test.h"

// pool allocator isn't suitable for stl container, so we don't test it here
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/FreeListAllocator.h"
#include "utils/allocators/MemoryPool.h"
#include "utils/allocators/GrowthPolicy.h"
#include "utils/allocators/HeapAllocator.h"
#include "utils/allocators/MonotonicAllocator.h"

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
