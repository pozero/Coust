#include "pch.h"

#include "test/Test.h"

#include "utils/containers/GrowthPolicy.h"

TEST_CASE("[Coust] [utils] [containers] GrowthPolicy (Power of 2 growth)" *
          doctest::skip(true)) {
    using namespace coust::container;
    size_t size = 10;
    detail::power_of_two_growth<8> growth{size};
    CHECK(size == 16);
    CHECK(growth.hash_to_index(size_t{16 + 15}) == size_t{15});
    CHECK(growth.hash_to_index(size_t{256 + 15}) == size_t{15});
    CHECK(growth.grow() == size_t{16 * 8});
    CHECK(growth.next_idx(14) == 15);
    CHECK(growth.next_idx(15) == 0);
    growth.clear();
    CHECK(growth.hash_to_index(size_t{16 + 15}) == size_t{0});
    CHECK(growth.hash_to_index(size_t{256 + 15}) == size_t{0});
}
