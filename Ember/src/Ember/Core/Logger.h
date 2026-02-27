#pragma once

#include <string>

namespace Ember {

	enum class LogLevel
	{
		Trace = 0,
		Info,
		Warn,
		Error,
		Fatal
	};

	class Logger
	{
	public:
		Logger() = default;
		~Logger() = default;

		void Log(LogLevel logLevel, const std::string& message);

		static Logger* CoreLogger()
		{
			static Logger coreLogger;
			return &coreLogger;
		}

		static Logger* ClientLogger()
		{
			static Logger clientLogger;
			return &clientLogger;
		}

	private:
		const char* GetLogLevelString(LogLevel logLevel);
		const char* GetLogLevelColor(LogLevel logLevel);
	};

}

#define EB_CORE_TRACE(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Trace, __VA_ARGS__)
#define EB_CORE_INFO(...)  Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Info, __VA_ARGS__)
#define EB_CORE_WARN(...)  Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Warn, __VA_ARGS__)
#define EB_CORE_ERROR(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Error, __VA_ARGS__)
#define EB_CORE_FATAL(...) Ember::Logger::CoreLogger()->Log(Ember::LogLevel::Fatal, __VA_ARGS__)

#define EB_TRACE(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Trace, __VA_ARGS__)
#define EB_INFO(...)  Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Info, __VA_ARGS__)
#define EB_WARN(...)  Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Warn, __VA_ARGS__)
#define EB_ERROR(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Error, __VA_ARGS__)
#define EB_FATAL(...) Ember::Logger::ClientLogger()->Log(Ember::LogLevel::Fatal, __VA_ARGS__)