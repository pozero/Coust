#pragma once

#include "utils/Compiler.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

WARNING_PUSH
DISABLE_ALL_WARNING
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
WARNING_POP

namespace coust {

class Logger {
public:
    Logger() = delete;
    Logger(Logger&&) = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;

public:
    // since we use async logger here, we must ensure all the messages are
    // flushed to screen before terminate or debug break in assertion \ panic
    static void flush_all() noexcept;
    static void shutdown_all() noexcept;

public:
    enum class Target {
        file = 0x1,
        std_out = 0x2,
        std_err = 0x4,

        mt = 0x8,
        mt_file = mt | file,
        mt_std_out = mt | std_out,
        mt_std_err = mt | std_err,
    };

    enum class Pattern {
        date = 0x1,
        iso_time = 0x2,
        mili_sec = 0x4,
        thread_id = 0x8,
        level = 0x10,
        logger_name = 0x20,
        file_name_line = 0x40,
        func_name = 0x80,
    };

    Logger(std::string_view name, Target target, Pattern pattern) noexcept;

    ~Logger() noexcept;

    spdlog::async_logger& get() noexcept;

    const spdlog::async_logger& get() const noexcept;

    std::weak_ptr<spdlog::async_logger> ptr() noexcept;

    const std::weak_ptr<spdlog::async_logger> ptr() const noexcept;

    // unsafe raw pointer getter
    spdlog::async_logger* raw_ptr() noexcept;

    // unsafe raw pointer getter
    const spdlog::async_logger* raw_ptr() const noexcept;

private:
    std::shared_ptr<spdlog::async_logger> m_logger;
    std::string_view m_name;
};

Logger& get_core_logger() noexcept;

}  // namespace coust
