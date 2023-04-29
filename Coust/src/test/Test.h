#pragma once

#include <utility>

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "doctest/doctest.h"
WARNING_POP

namespace coust {

WARNING_PUSH
#if defined(__clang__)
    #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
// all commands related to doctest are placed after "-doctest" option
inline std::pair<int, const char* const*> parse_test_cmds(
    int argc, const char* const* argv) {
    const char* const doctest_cmd_start = "-doctest";
    for (int i = 0; i < argc - 1; ++i) {
        if (strcmp(doctest_cmd_start, argv[i]) == 0)
            return {argc - i - 1, &argv[i + 1]};
    }
    return {0, nullptr};
}
WARNING_POP

inline std::pair<bool, int> run_tests(int argc, const char* const* argv) {
    doctest::Context ctx{argc, argv};
    const int result = ctx.run();
    return {ctx.shouldExit(), result};
}

}  // namespace coust