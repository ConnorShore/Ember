#pragma once

#include <string>

namespace Ember {

	enum class LogLevel
	{
		Trace = 0,
		Debug,
		Info,
		Warn,
		Error
	};

	class Logger
	{
	public:
		void Log(LogLevel logLevel, const std::string& message);

		static Logger* CoreLogger() { return m_CoreInstance == nullptr ? m_CoreInstance = new Logger() : m_CoreInstance; }
		static Logger* ClientLogger() { return m_ClientInstance == nullptr ? m_ClientInstance = new Logger() : m_ClientInstance; }

	private:
		const char* GetLogLevelString(LogLevel logLevel);
		const char* GetLogLevelColor(LogLevel logLevel);

	private:
		static Logger* m_CoreInstance;
		static Logger* m_ClientInstance;
	};

}

#define EB_CORE_TRACE(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Trace, __VA_ARGS__)
#define EB_CORE_DEBUG(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Debug, __VA_ARGS__)
#define EB_CORE_INFO(...)  Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Info, __VA_ARGS__)
#define EB_CORE_WARN(...)  Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Warn, __VA_ARGS__)
#define EB_CORE_ERROR(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Error, __VA_ARGS__)

#define EB_TRACE(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Trace, __VA_ARGS__)
#define EB_DEBUG(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Debug, __VA_ARGS__)
#define EB_INFO(...)  Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Info, __VA_ARGS__)
#define EB_WARN(...)  Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Warn, __VA_ARGS__)
#define EB_ERROR(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Error, __VA_ARGS__)