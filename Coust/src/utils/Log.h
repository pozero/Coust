#pragma once

#include "core/Logger.h"

#define COUST_TRACE(...)                                                       \
    SPDLOG_LOGGER_TRACE((get_core_logger().ptr().lock()), __VA_ARGS__)
#define COUST_INFO(...)                                                        \
    SPDLOG_LOGGER_INFO((get_core_logger().ptr().lock()), __VA_ARGS__)
#define COUST_WARN(...)                                                        \
    SPDLOG_LOGGER_WARN((get_core_logger().ptr().lock()), __VA_ARGS__)
#define COUST_ERROR(...)                                                       \
    SPDLOG_LOGGER_ERROR((get_core_logger().ptr().lock()), __VA_ARGS__)
#define COUST_CRITICAL(...)                                                    \
    SPDLOG_LOGGER_CRITICAL((get_core_logger().ptr().lock()), __VA_ARGS__)
