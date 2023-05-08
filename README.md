# Coust

The old Coust isn't written with c++20 and all its good stuff in mind. So I decided to rewrite the whole project with a more modern approach (hope so), and that should be a lot of fun :)

## Rewriting plan / TODO

-   Switch build system to CMake
-   Speed up build time
    -   [Build Time trace](https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-ftime-trace)
    -   Building cache: [sccahe](https://github.com/mozilla/sccache)
-   Rust-like `result` or C++23-like `std::expected` error handling style
-   More adoption of STL algorithm
-   More lambda
-   More compact coding style
    Not this
    ```C++
    [[nodiscard]] constexpr int Add(int l, int r) noexcept
    {
        return l + r;
    }
    ```
    but this
    ```C++
    [[nodiscard]] constexpr int add(int l, int r) noexcept {
        return l + r;
    }
    ```
-   `constexpr` everything! (not really)
    -   There's an important use case of compile-time programming -- enums. Lot's of time we just want to transform a enum literal to a library specific type (like to a vulkan enum) and we can completely get rid of any run-time overhead (even though they might be just several instructions) when use our own enum, which is great.
-   `noexcept` and `const` everything!
-   Give `[[likely]]`, `[[unlikely]]`, restrict and `[[assume()]]` a chance
    -   `[[assume()]]` is a new feature presented in C++23 which isn't fully available yet, but we have compiler extensions:
        -   MSVC: [`__assume()`](https://learn.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-170)
        -   clang: [`__builtin_assume()`](https://clang.llvm.org/docs/LanguageExtensions.html#langext-builtin-assume)
    -   restrict isn't part of C++ standard, but it's supported by many compilers
        -   MSVC: [`__restrict`](https://learn.microsoft.com/en-us/cpp/cpp/extension-restrict?view=msvc-170)
-   Radically optimization: Fast math, no run-time type information and no exception
-   Replacing `std::unordered_map` (implemented in link list) with robin map
-   Vulkan hpp bind (that's the number one reason why I want to rewrite this project)
-   Job system