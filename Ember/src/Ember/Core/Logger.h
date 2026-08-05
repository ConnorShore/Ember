#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <vector>
#include <cstdint>

namespace Ember {

	enum class LogLevel
	{
		Trace = 0,
		Info,
		Warn,
		Error,
		Fatal
	};

	static constexpr size_t LogLevelCount = 5;

	// Kept structured rather than pre-formatted so the editor can colour and filter it.
	struct LogRecord
	{
		LogLevel Level = LogLevel::Info;
		std::string Logger;
		std::string Message;
		std::chrono::system_clock::time_point Time;
		uint64_t Sequence = 0;
	};

	class Logger
	{
	public:
		Logger(const std::string& name) : m_Name(name) {}
		~Logger() = default;

		static void InitFileLogging(const std::string& filepath);

		// Appends every record newer than cursor and then advances it. Non-destructive, so multiple
		// consumers can each hold their own cursor and a late consumer still sees startup lines.
		static void DrainRecords(uint64_t& cursor, std::vector<LogRecord>& out);

		template <typename... Args>
		void Log(LogLevel logLevel, std::format_string<Args...> fmt, Args&&... args)
		{
			std::string userMessage = std::format(fmt, std::forward<Args>(args)...);

			Logger::PushRecord(logLevel, m_Name, userMessage);

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
		static void PushRecord(LogLevel level, const std::string& loggerName, const std::string& message);

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