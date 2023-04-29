#pragma once

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
    #define WARNING_PUSH _Pragma("clang diagnostic push")
    #define WARNING_POP _Pragma("clang diagnostic pop")
    #define DISABLE_ALL_WARNING                                                \
        _Pragma("clang diagnostic ignored \"-Weverything\"")
    #if __has_builtin(__builtin_debugtrap)
        #define DEBUG_BREAK() __builtin_debugtrap()
    #else
        #define DEBUG_BREAK()
    #endif
#elif defined(_MSC_VER)
    #define RESTRICT __restrict
    #define ASSUME(exp) (__assume(exp))
    #define PRETTY_FUNC __FUNCSIG__
    // There no such thing as "disabling all warning" in msvc, so the macro is
    // lying here
    #define WARNING_PUSH _Pragma("warning( push, 0 )")
    #define WARNING_POP _Pragma("warning( pop )")
    #define DISABLE_ALL_WARNING
    #define DEBUG_BREAK() __debugbreak()
#else
    #error Unsupported compiler
#endif
