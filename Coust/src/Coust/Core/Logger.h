#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Coust
{
	class Logger
	{
	public:
		Logger(const Logger&) = delete;
		Logger& operator=(Logger&&) = delete;
		Logger& operator=(const Logger&) = delete;

	public:
		Logger() noexcept = default;
		Logger(Logger&&) noexcept = default;

#undef stdout
#undef stderr
		Logger(const char* fileName, const char* pattern, bool outputToStdout) noexcept;

		~Logger() noexcept;

		spdlog::logger& Get() const noexcept;
	
	private:
		std::unique_ptr<spdlog::logger> m_Log{};

	public:
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
	#define COUST_CORE_ASSERT(x, ...)		do { if (!(x)) [[unlikely]] { COUST_CORE_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); __debugbreak(); }} while (false)
	#define COUST_ASSERT(x, ...)			do { if (!(x)) [[unlikely]] { COUST_ERROR("Assertion Failed:\n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); __debugbreak(); }} while (false)
	#define COUST_CORE_PANIC_IF(x, ...) 	do { if ((x)) [[unlikely]] { COUST_CORE_CRITICAL("Panic: \n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); __debugbreak(); }} while (false)
	#define COUST_TODO(...)					do { COUST_CORE_ERROR("Code Unfinished:\n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); __debugbreak(); } while (false)
#else
	#define COUST_CORE_ASSERT(x, ...)
	#define COUST_ASSERT(x, ...)
	#define COUST_CORE_PANIC_IF(x, ...) 	do { if ((x)) [[unlikely]] { COUST_CORE_CRITICAL("Panic: \n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, fmt::format(__VA_ARGS__)); std::abort(); }} while (false)
	#define COUST_TODO(...)					do { COUST_CORE_ERROR("Code Unfinished:\n\t{0}, {1}\n\t{2}\n\t", __FILE__, __LINE__, __VA_ARGS__); std::abort(); } while (false)
#endif

