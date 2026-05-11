#pragma once

#include <iostream>
#include <string>
#include <fstream>

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
		Logger(const std::string& name) : m_Name(name) {}
		~Logger() = default;

		static void InitFileLogging(const std::string& filepath);

		template <typename... Args>
		void Log(LogLevel logLevel, std::format_string<Args...> fmt, Args&&... args)
		{
			std::string userMessage = std::format(fmt, std::forward<Args>(args)...);

			// Clean output for the file (No color codes!)
			std::string cleanOutput = std::format("{}: [{}] {}\n",
				m_Name,
				GetLogLevelString(logLevel),
				userMessage);

			// Colored output for the console
			std::string consoleOutput = std::format("{}{}{}",
				GetLogLevelColor(logLevel),
				cleanOutput,
				GetLogLevelResetColor());

			std::cout << consoleOutput;

			if (s_LogFile.is_open())
			{
				s_LogFile << cleanOutput;
				s_LogFile.flush(); // CRITICAL: Flush immediately so if the game crashes on the next line, the log is saved!
			}
		}

		static Logger* CoreLogger()
		{
			static Logger coreLogger("ENGINE");
			return &coreLogger;
		}

		static Logger* ClientLogger()
		{
			static Logger clientLogger("APP");
			return &clientLogger;
		}

	private:
		const char* GetLogLevelString(LogLevel logLevel);
		const char* GetLogLevelColor(LogLevel logLevel);
		const char* GetLogLevelResetColor();

	private:
		std::string m_Name;
		static std::ofstream s_LogFile;
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