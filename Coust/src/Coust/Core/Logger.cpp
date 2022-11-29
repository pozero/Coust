#include "Coust/Core/Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Coust
{
	std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

	bool Logger::Initialize()
	{
		bool ret = false;
		bool coreLoggerCreated = false, clientLoggerCreated = false;

		std::vector<spdlog::sink_ptr> sinks;
		sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Coust.log", true));

		sinks[0]->set_pattern("%^[%T.%e] %n: %v%$");
		sinks[1]->set_pattern("[%T.%e] [%l] %n: %v");

		s_CoreLogger = std::make_shared<spdlog::logger>("COUST", begin(sinks), end(sinks));
		if (s_CoreLogger)
		{
			spdlog::register_logger(s_CoreLogger);
			s_CoreLogger->set_level(spdlog::level::trace);
			s_CoreLogger->flush_on(spdlog::level::trace);

			coreLoggerCreated = true;
		}
		else
			COUST_CORE_ERROR("Core Logger Initialization Failed");

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(sinks), end(sinks));
		if (s_ClientLogger)
		{
			spdlog::register_logger(s_ClientLogger);
			s_ClientLogger->set_level(spdlog::level::trace);
			s_ClientLogger->flush_on(spdlog::level::trace);
			
			clientLoggerCreated = true;
		}
		else
			COUST_CORE_ERROR("Client Logger Initialization Failed");

		if (coreLoggerCreated && clientLoggerCreated)
		{
			COUST_CORE_INFO("Logger Initialized");
			ret = true;
		}

		return ret;
	}

	void Logger::Shutdown()
	{
		if (s_CoreLogger)
			s_CoreLogger->flush();
		if (s_ClientLogger)
			s_ClientLogger->flush();

		spdlog::drop_all();
		spdlog::shutdown();
	}

}
