#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Coust
{
	class Logger
	{
	public:
		Logger() = default;
		~ Logger() = default;

		[[nodiscard]] static bool Initialize();
		static void Shutdown();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define COUST_CORE_TRACE(...)			::Coust::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define COUST_CORE_INFO(...)			::Coust::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define COUST_CORE_WARN(...)			::Coust::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define COUST_CORE_ERROR(...)			::Coust::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define COUST_CORE_CRITICAL(...)		::Coust::Logger::GetCoreLogger()->critical(__VA_ARGS__)

#define COUST_TRACE(...)				::Coust::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define COUST_INFO(...)					::Coust::Logger::GetClientLogger()->info(__VA_ARGS__)
#define COUST_WARN(...)					::Coust::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define COUST_ERROR(...)				::Coust::Logger::GetClientLogger()->error(__VA_ARGS__)
#define COUST_CRITICAL(...)				::Coust::Logger::GetClientLogger()->critical(__VA_ARGS__)

#ifndef COUST_FULL_RELEASE
	#define COUST_CORE_ASSERT(x, ...)		{ if (!(x)) { COUST_CORE_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}", __FILE__, __LINE__, __VA_ARGS__); __debugbreak(); }}
	#define COUST_ASSERT(x, ...)			{ if (!(x)) { COUST_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}", __FILE__, __LINE__, __VA_ARGS__); __debugbreak(); }}
#else
	#define COUST_CORE_ASSERT(x, ...)		{ if (!(x)) { COUST_CORE_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}", __FILE__, __LINE__, __VA_ARGS__); ::Coust::Application::GetInstance().Close(); }}
	#define COUST_ASSERT(x, ...)			{ if (!(x)) { COUST_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}", __FILE__, __LINE__, __VA_ARGS__); ::Coust::Application::GetInstance().Close(); }}
#endif
