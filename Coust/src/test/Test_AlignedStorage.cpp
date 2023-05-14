#include "pch.h"

#include "test/Test.h"

#include "utils/AlignedStorage.h"

TEST_CASE("[Coust] [utils] AlingedStorage" * doctest::skip(true)) {
    using namespace coust;

    SUBCASE("single thread") {
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

    SUBCASE("multithread") {
        class Obj {
        public:
            Obj(int* pcc, int* pdc) noexcept
                : construct_count(pcc), destruct_count(pdc) {
                (*construct_count)++;
            }
            ~Obj() noexcept { (*destruct_count)++; }

        private:
            int* construct_count;
            int* destruct_count;
        };
        int construct_cnt = 0, destruct_cnt = 0;
        {
            coust::AlignedStorage<Obj> shared_data{};
            auto init_data = [pdata = &shared_data, pcc = &construct_cnt,
                                 pdc = &destruct_cnt]() {
                pdata->initialize(pcc, pdc);
            };
            std::jthread t0{init_data};
            std::jthread t1{init_data};
            std::jthread t2{init_data};
            std::jthread t3{init_data};
            std::jthread t4{init_data};
        }
        CHECK(construct_cnt == 1);
        CHECK(destruct_cnt == 1);
    }
}
