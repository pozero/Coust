#include "pch.h"

#include "test/Test.h"

#include "utils/allocators/MemoryPool.h"
#include "utils/allocators/GrowthPolicy.h"
#include "utils/allocators/SmartPtr.h"
#include "utils/allocators/FreeListAllocator.h"

TEST_CASE("[Coust] [utils] [allocators] Smart Pointer" * doctest::skip(true)) {
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
