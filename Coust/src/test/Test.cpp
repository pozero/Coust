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
TEST_CASE("[Coust] [utils] Alinged Storage") {
    using namespace coust;
    struct foo {
        int i;
        float f;
        char c;
        void* p;

        foo(int ii, float ff, char cc, void* pp) noexcept
            : i(ii), f(ff), c(cc), p(pp) {}

        foo(const foo& other) noexcept = default;

        foo(foo&& other) noexcept = default;

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
    AlignedStorage<bar> bb{};
    auto const ff = foo{1, 1.0f, 'a', nullptr};
    bb.initialize(ff, foo{2, 2.0f, 'b', nullptr}, 3);
    CHECK(bb.get().f1.i == 1);
    CHECK(bb.get().f1.f == 1.0f);
    CHECK(bb.get().f1.c == 'a');
    CHECK(bb.get().f1.p == nullptr);
    CHECK(bb.get().f1.emm() == 3);
    CHECK(bb.get().f2.i == 2);
    CHECK(bb.get().f2.f == 2.0f);
    CHECK(bb.get().f2.c == 'b');
    CHECK(bb.get().f2.p == nullptr);
    CHECK(bb.get().f2.emm() == 6);
    CHECK(bb.get().i3 == 3);
}
#endif
