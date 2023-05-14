#pragma once

#include "test/Test.h"

#include "core/Logger.h"
#include "utils/Enums.h"

namespace coust {
namespace detail {
static std::string get_logger_pattern(Logger::Pattern pattern) noexcept;
}  // namespace detail
}  // namespace coust

TEST_CASE("[Coust] [core] Logger" * doctest::skip(true)) {
    using namespace coust;
    auto p1 = detail::get_logger_pattern(Logger::Pattern::logger_name);
    CHECK(p1 == std::string{"%^%n: %v%$"});
    auto p2 = detail::get_logger_pattern(coust::merge(Logger::Pattern::iso_time,
        Logger::Pattern::mili_sec, Logger::Pattern::level));
    CHECK(p2 == std::string{"%^[%T.%e] [%l] %v%$"});
    auto p3 = detail::get_logger_pattern(coust::merge(Logger::Pattern::date,
        Logger::Pattern::iso_time, Logger::Pattern::mili_sec,
        Logger::Pattern::thread_id, Logger::Pattern::logger_name));
    CHECK(p3 == std::string{"%^[%D %T.%e] <%t> %n: %v%$"});
    auto p4 =
        detail::get_logger_pattern(coust::merge(Logger::Pattern::file_name_line,
            Logger::Pattern::func_name, Logger::Pattern::logger_name));
    CHECK(p4 == std::string{"%^(%s:%# %!) %n: %v%$"});
}
