#include "pch.h"

#include "core/GlobalContext.h"

#include "utils/Enums.h"
#include "utils/Compiler.h"

namespace coust {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wexit-time-destructors")
GlobalContext& GlobalContext::get() noexcept {
    static GlobalContext s_global_ctx(
        merge(Logger::Target::mt_file, Logger::Target::mt_std_out),
        merge(Logger::Pattern::iso_time, Logger::Pattern::mili_sec,
            Logger::Pattern::level, Logger::Pattern::file_name_line,
            Logger::Pattern::logger_name));
    return s_global_ctx;
}
WARNING_POP

GlobalContext::GlobalContext(Logger::Target core_logger_target,
    Logger::Pattern core_logger_pattern) noexcept
    : m_core_logger("Coust", core_logger_target, core_logger_pattern) {
}

GlobalContext::~GlobalContext() noexcept {
}

Logger& GlobalContext::get_core_logger() noexcept {
    return m_core_logger;
}

}  // namespace coust
