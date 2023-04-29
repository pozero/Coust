#include "Coust.h"

namespace coust {

std::unique_ptr<Application> create_application() {
    auto pat = coust::merge(Logger::Pattern::date, Logger::Pattern::iso_time,
        Logger::Pattern::thread_id, Logger::Pattern::level,
        Logger::Pattern::file_name_line, Logger::Pattern::func_name,
        Logger::Pattern::logger_name);
    Logger logger{"Test", Logger::Target::std_out, pat};
    SPDLOG_LOGGER_INFO(logger.raw_ptr(), "How's it going?");
    return std::make_unique<Application>();
}

}  // namespace coust
