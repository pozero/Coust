#include "pch.h"

#include "core/Logger.h"
#include "core/Memory.h"
#include "utils/allocators/SmartPtr.h"
#include "utils/Enums.h"
#include "utils/Compiler.h"

namespace coust {
namespace detail {

static std::string get_logger_pattern(Logger::Pattern pattern) noexcept {
    // full format:
    // %^[time] <thread_id> [level] (file:line func) logger_name: message %$

    std::array<std::string_view, 19> constexpr symbols = {"%^", "[", "%D ",
        "%T", ".%e", "] ", "<", "%t", "> ", "[", "%l", "] ", "(", "%s:%# ",
        "%!", ") ", "%n", ": ", "%v%$"};
    size_t constexpr max_result_size =
        std::accumulate(symbols.cbegin(), symbols.cend(), size_t{0u},
            [](size_t s, std::string_view sv) { return sv.size() + s; });

    uint32_t constexpr begin = 0;

    uint32_t constexpr time_begin = 1;
    uint32_t constexpr date = 2;
    uint32_t constexpr iso_time = 3;
    uint32_t constexpr mili_sec = 4;
    uint32_t constexpr time_end = 5;

    uint32_t constexpr thread_id_begin = 6;
    uint32_t constexpr thread_id = 7;
    uint32_t constexpr thread_id_end = 8;

    uint32_t constexpr level_begin = 9;
    uint32_t constexpr level = 10;
    uint32_t constexpr level_end = 11;

    uint32_t constexpr macro_begin = 12;
    uint32_t constexpr file_name_line = 13;
    uint32_t constexpr func_name = 14;
    uint32_t constexpr macro_end = 15;

    uint32_t constexpr logger_name = 16;
    uint32_t constexpr logger_name_end = 17;

    uint32_t constexpr end = 18;

    std::array<bool, symbols.size()> applied_symbols{};
    applied_symbols[begin] = true;
    applied_symbols[end] = true;

    if (has(pattern, Logger::Pattern::date)) {
        applied_symbols[time_begin] = true;
        applied_symbols[date] = true;
        applied_symbols[time_end] = true;
    }
    if (has(pattern, Logger::Pattern::iso_time)) {
        applied_symbols[time_begin] = true;
        applied_symbols[iso_time] = true;
        applied_symbols[time_end] = true;
    }
    if (has(pattern, Logger::Pattern::mili_sec)) {
        applied_symbols[time_begin] = true;
        applied_symbols[mili_sec] = true;
        applied_symbols[time_end] = true;
    }
    if (has(pattern, Logger::Pattern::thread_id)) {
        applied_symbols[thread_id_begin] = true;
        applied_symbols[thread_id] = true;
        applied_symbols[thread_id_end] = true;
    }
    if (has(pattern, Logger::Pattern::level)) {
        applied_symbols[level_begin] = true;
        applied_symbols[level] = true;
        applied_symbols[level_end] = true;
    }
    if (has(pattern, Logger::Pattern::logger_name)) {
        applied_symbols[logger_name] = true;
        applied_symbols[logger_name_end] = true;
    }
    if (has(pattern, Logger::Pattern::file_name_line)) {
        applied_symbols[macro_begin] = true;
        applied_symbols[file_name_line] = true;
        applied_symbols[macro_end] = true;
    }
    if (has(pattern, Logger::Pattern::func_name)) {
        applied_symbols[macro_begin] = true;
        applied_symbols[func_name] = true;
        applied_symbols[macro_end] = true;
    }
    std::string res{};
    res.reserve(max_result_size);
    for (auto i : std::views::iota(0u, symbols.size())) {
        if (applied_symbols[i])
            res += symbols[i];
    }
    return res;
}

}  // namespace detail

void Logger::flush_all() noexcept {
    spdlog::flush_on(spdlog::level::level_enum::trace);
}

void Logger::shutdown_all() noexcept {
    spdlog::drop_all();
    spdlog::shutdown();
}

Logger::Logger(std::string_view name, Target target, Pattern pattern) noexcept
    : m_name(name) {
    // we just use default thread pool for spdlog::logger
    // https://github.com/gabime/spdlog/wiki/6.-Asynchronous-logging#spdlogs-thread-pool
    [[maybe_unused]] static bool spdlog_tpool_inited =
        std::invoke([]() -> bool {
            spdlog::init_thread_pool(8192, 2);
            return true;
        });

    using namespace spdlog;
    std::array<sink_ptr, 3> sinks{};
    uint32_t sinks_count = 0;
    std::string file_name{name};
    file_name += ".log";

    bool const is_multithreading = has(target, Target::mt);
    if (has(target, Target::file)) {
        if (is_multithreading)
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::basic_file_sink_mt>(
                    get_default_alloc(), file_name, true);
        else
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::basic_file_sink_st>(
                    get_default_alloc(), file_name, true);
    }

    if (has(target, Target::std_out)) {
        if (is_multithreading)
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::stdout_color_sink_mt>(
                    get_default_alloc());
        else
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::stdout_color_sink_st>(
                    get_default_alloc());
    }

    if (has(target, Target::std_err)) {
        if (is_multithreading)
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::stderr_color_sink_mt>(
                    get_default_alloc());
        else
            sinks[sinks_count++] =
                memory::allocate_shared<sinks::stderr_color_sink_st>(
                    get_default_alloc());
    }

    m_logger = memory::allocate_shared<async_logger>(get_default_alloc(),
        name.data(), begin(sinks), begin(sinks) + sinks_count, thread_pool());
    auto pat = detail::get_logger_pattern(pattern);
    m_logger->set_pattern(pat);
}

Logger::~Logger() noexcept {
}

spdlog::async_logger& Logger::get() noexcept {
    return *m_logger;
}

const spdlog::async_logger& Logger::get() const noexcept {
    return *m_logger;
}

std::weak_ptr<spdlog::async_logger> Logger::ptr() noexcept {
    return m_logger;
}

const std::weak_ptr<spdlog::async_logger> Logger::ptr() const noexcept {
    return m_logger;
}

spdlog::async_logger* Logger::raw_ptr() noexcept {
    return m_logger.get();
}

const spdlog::async_logger* Logger::raw_ptr() const noexcept {
    return m_logger.get();
}

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wexit-time-destructors")
Logger& get_core_logger() noexcept {
    static Logger s_core_logger{"Coust",
        merge(Logger::Target::mt_file, Logger::Target::mt_std_out),
        merge(Logger::Pattern::iso_time, Logger::Pattern::mili_sec,
            Logger::Pattern::level, Logger::Pattern::file_name_line,
            Logger::Pattern::logger_name)};
    return s_core_logger;
}
WARNING_POP

}  // namespace coust

#if defined(COUST_TEST)
    #include "test/Test_Logger_static.h"
#endif
