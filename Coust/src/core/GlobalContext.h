#pragma once

#include "core/Logger.h"

namespace coust {
class GlobalContext {
public:
    GlobalContext() = delete;
    GlobalContext(GlobalContext&&) = delete;
    GlobalContext(const GlobalContext&) = delete;
    GlobalContext& operator=(GlobalContext&&) = delete;
    GlobalContext& operator=(const GlobalContext&) = delete;

public:
    static GlobalContext& get() noexcept;

public:
    GlobalContext(Logger::Target core_logger_target,
        Logger::Pattern core_logger_pattern) noexcept;

    ~GlobalContext() noexcept;

    Logger& get_core_logger() noexcept;

private:
    Logger m_core_logger;
};
}  // namespace coust
