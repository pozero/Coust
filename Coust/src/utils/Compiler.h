#pragma once

#define COUST_IMPL_DO_PRAGMA(x) _Pragma(#x)

// It seems that compilation using clang under windows environment would also
// include the `_MSC_VER` macro, so we should check `__clang__` macro first.
#if defined(__clang__)
    #define RESTRICT __restrict__
    #if __has_builtin(__builtin_assume)
        #define ASSUME(exp) (__builtin_assume(exp))
    #else
        #define ASSUME(exp)
    #endif
    #define PRETTY_FUNC __PRETTY_FUNCTION__
    #define WARNING_PUSH COUST_IMPL_DO_PRAGMA(clang diagnostic push)
    #define WARNING_POP COUST_IMPL_DO_PRAGMA(clang diagnostic pop)
    #define DISABLE_ALL_WARNING                                                \
        COUST_IMPL_DO_PRAGMA(clang diagnostic ignored "-Weverything")
    #define CLANG_DISABLE_WARNING(warn)                                        \
        COUST_IMPL_DO_PRAGMA(clang diagnostic ignored warn)
    #define MSVC_DISABLE_WARNING(warn)
    #if __has_builtin(__builtin_debugtrap)
        #define DEBUG_BREAK() __builtin_debugtrap()
    #else
        #define DEBUG_BREAK()
    #endif
    #define FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define RESTRICT __restrict
    #define ASSUME(exp) (__assume(exp))
    #define PRETTY_FUNC __FUNCSIG__
    #define WARNING_PUSH COUST_IMPL_DO_PRAGMA(warning(push, 0))
    #define WARNING_POP COUST_IMPL_DO_PRAGMA(warning(pop))
    // There no such thing as "disabling all warning" in msvc, so the macro is
    // lying here
    #define DISABLE_ALL_WARNING
    #define CLANG_DISABLE_WARNING(warn)
    #define MSVC_DISABLE_WARNING(warn)                                         \
        COUST_IMPL_DO_PRAGMA(warning(disable : warn))
    #define DEBUG_BREAK() __debugbreak()
    #define FORCE_INLINE __forceinline
#else
    #error Unsupported compiler
#endif
