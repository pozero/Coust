#include "pch.h"

#include "test/Test.h"

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/containers/RobinSet.h"
#include "utils/containers/RobinMap.h"

TEST_CASE("[Coust] [core] Global Memory Allocation" * doctest::skip(true)) {
    using namespace coust;
    uint32_t constexpr exp_cnt = 100;
    {
        memory::robin_set<uint32_t, DefaultAlloc> set0{get_default_alloc()};
        for (uint32_t i = 0; i < exp_cnt; ++i) {
            set0.insert(i);
            set0.insert(i);
            set0.insert(i);
        }
        CHECK(set0.size() == exp_cnt);
        for (uint32_t i = 0; i < exp_cnt; ++i) {
            CHECK(set0.contains(i));
        }
    }
    {
        memory::robin_set_nested<memory::string<DefaultAlloc>, DefaultAlloc>
            set1{get_default_alloc()};
        for (uint32_t i = 0; i < exp_cnt; ++i) {
            memory::string<DefaultAlloc> str{
                std::to_string(i), get_default_alloc()};
            set1.insert(str);
            set1.insert(str);
            set1.insert(str);
        }
        CHECK(set1.size() == exp_cnt);
        for (uint32_t i = 0; i < exp_cnt; ++i) {
            memory::string<DefaultAlloc> str{
                std::to_string(i), get_default_alloc()};
            CHECK(set1.contains(str));
        }
    }
}
