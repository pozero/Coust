#pragma once

#include "core/Logger.h"
#include "core/GlobalContext.h"

#define COUST_TRACE(...)                                                       \
    SPDLOG_LOGGER_TRACE(                                                       \
        GlobalContext::get().get_core_logger().raw_ptr(), __VA_ARGS__)
#define COUST_INFO(...)                                                        \
    SPDLOG_LOGGER_INFO(                                                        \
        GlobalContext::get().get_core_logger().raw_ptr(), __VA_ARGS__)
#define COUST_WARN(...)                                                        \
    SPDLOG_LOGGER_WARN(                                                        \
        GlobalContext::get().get_core_logger().raw_ptr(), __VA_ARGS__)
#define COUST_ERROR(...)                                                       \
    SPDLOG_LOGGER_ERROR(                                                       \
        GlobalContext::get().get_core_logger().raw_ptr(), __VA_ARGS__)
#define COUST_CRITICAL(...)                                                    \
    SPDLOG_LOGGER_CRITICAL(                                                    \
        GlobalContext::get().get_core_logger().raw_ptr(), __VA_ARGS__)
