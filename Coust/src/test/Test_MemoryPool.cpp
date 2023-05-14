#include "pch.h"

#include "test/Test.h"

#include "utils/allocators/MemoryPool.h"

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
