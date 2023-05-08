#pragma once

#include "core/Logger.h"
#include "core/GlobalContext.h"
#include "utils/Compiler.h"

#include <exception>

#if defined(COUST_REL)
    #define COUST_ASSERT(exp, ...) (void) 0
    #define COUST_PANIC_IF(exp, ...)                                           \
        do {                                                                   \
            if ((exp)) [[unlikely]] {                                          \
                SPDLOG_LOGGER_CRITICAL(                                        \
                    GlobalContext::get().get_core_logger().raw_ptr(),          \
                    "True condition PANIC: {}\n\t{}", #exp,                    \
                    fmt::format(__VA_ARGS__));                                 \
                Logger::flush_all();                                           \
                Logger::shutdown_all();                                        \
                std::terminate();                                              \
            }                                                                  \
        } while (false)
    #define COUST_PANIC_IF_NOT(exp, ...)                                       \
        do {                                                                   \
            if (!(exp)) [[unlikely]] {                                         \
                SPDLOG_LOGGER_CRITICAL(                                        \
                    GlobalContext::get().get_core_logger().raw_ptr(),          \
                    "False condition PANIC: {}\n\t{}", #exp,                   \
                    fmt::format(__VA_ARGS__));                                 \
                Logger::flush_all();                                           \
                Logger::shutdown_all();                                        \
                std::terminate();                                              \
            }                                                                  \
        } while (false)
#else
    #define COUST_ASSERT(exp, ...)                                             \
        do {                                                                   \
            if (!(exp)) [[unlikely]] {                                         \
                SPDLOG_LOGGER_ERROR(                                           \
                    GlobalContext::get().get_core_logger().raw_ptr(),          \
                    "Assertion {} failed\n\t{}", #exp,                         \
                    fmt::format(__VA_ARGS__));                                 \
                Logger::flush_all();                                           \
                DEBUG_BREAK();                                                 \
            }                                                                  \
        } while (false)
    #define COUST_PANIC_IF(exp, ...)                                           \
        do {                                                                   \
            if ((exp)) [[unlikely]] {                                          \
                SPDLOG_LOGGER_CRITICAL(                                        \
                    GlobalContext::get().get_core_logger().raw_ptr(),          \
                    "True condition PANIC: {}\n\t{}", #exp,                    \
                    fmt::format(__VA_ARGS__));                                 \
                Logger::flush_all();                                           \
                DEBUG_BREAK();                                                 \
            }                                                                  \
        } while (false)
    #define COUST_PANIC_IF_NOT(exp, ...)                                       \
        do {                                                                   \
            if (!(exp)) [[unlikely]] {                                         \
                SPDLOG_LOGGER_CRITICAL(                                        \
                    GlobalContext::get().get_core_logger().raw_ptr(),          \
                    "False condition PANIC: {}\n\t{}", #exp,                   \
                    fmt::format(__VA_ARGS__));                                 \
                Logger::flush_all();                                           \
                DEBUG_BREAK();                                                 \
            }                                                                  \
        } while (false)
#endif
