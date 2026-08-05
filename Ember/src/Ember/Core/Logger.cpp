#include "ebpch.h"
#include "Logger.h"

#include <deque>
#include <mutex>

#define EB_COLOR_LOG_RESET "\033[0m"
#define EB_COLOR_LOG_TRACE "\033[1;34m"
#define EB_COLOR_LOG_INFO  "\033[1;32m"
#define EB_COLOR_LOG_WARN  "\033[1;33m"
#define EB_COLOR_LOG_ERROR "\033[1;41m"
#define EB_COLOR_LOG_FATAL "\033[1;45m"

#define EB_STRING_LOG_TRACE "TRACE"
#define EB_STRING_LOG_INFO  "INFO"
#define EB_STRING_LOG_WARN  "WARN"
#define EB_STRING_LOG_ERROR "ERROR"
#define EB_STRING_LOG_FATAL "FATAL"

// TODO: Abstract this out a bit to be able to customize the logger more in the future
//	Maybe add file logging and other and other color/formatting options in the future as well
//  Also include timestamps in the log messages in the future as well

namespace Ember {

	namespace {

		struct RecordBuffer
		{
			std::mutex Mutex;
			std::deque<LogRecord> Records;
			uint64_t NextSequence = 0;
		};

		// Roughly a megabyte at typical message lengths, and far more than a frame's worth of drain.
		constexpr size_t s_MaxRecords = 8192;

		// Function-local so a log call from another translation unit's static initialiser still
		// constructs this in time.
		RecordBuffer& Buffer()
		{
			static RecordBuffer s_Buffer;
			return s_Buffer;
		}

	}

	std::ofstream Logger::s_LogFile;

	void Logger::InitFileLogging(const std::string& filepath)
	{
		// Create the directory if it doesn't exist
		if (auto parentDir = std::filesystem::path(filepath).parent_path(); !std::filesystem::exists(parentDir))
		{
			std::filesystem::create_directories(parentDir);
		}

		// Open the file and truncate (clear) any old logs from previous runs
		s_LogFile.open(filepath, std::ios::out | std::ios::trunc);

		if (!s_LogFile.is_open())
		{
			std::cerr << "Failed to open log file: " << filepath << std::endl;
		}
	}

	void Logger::PushRecord(LogLevel level, const std::string& loggerName, const std::string& message)
	{
		RecordBuffer& buffer = Buffer();
		std::lock_guard lock(buffer.Mutex);

		// Oldest-out: consumers drain by sequence, so dropping the front only loses lines that a
		// consumer stalled for 8192 messages would have missed anyway.
		if (buffer.Records.size() >= s_MaxRecords)
			buffer.Records.pop_front();

		buffer.Records.push_back(LogRecord{
			.Level = level,
			.Logger = loggerName,
			.Message = message,
			.Time = std::chrono::system_clock::now(),
			.Sequence = buffer.NextSequence++
		});
	}

	void Logger::DrainRecords(uint64_t& cursor, std::vector<LogRecord>& out)
	{
		RecordBuffer& buffer = Buffer();
		std::lock_guard lock(buffer.Mutex);

		if (buffer.Records.empty() || cursor >= buffer.NextSequence)
		{
			cursor = buffer.NextSequence;
			return;
		}

		// Sequences only ever increment by one and only the front is dropped, so they stay contiguous
		// and the cursor converts straight to an index instead of needing a search.
		const uint64_t oldest = buffer.Records.front().Sequence;
		const size_t start = cursor > oldest ? static_cast<size_t>(cursor - oldest) : 0;

		out.insert(out.end(), buffer.Records.begin() + start, buffer.Records.end());
		cursor = buffer.NextSequence;
	}

	const char* Logger::GetLogLevelString(LogLevel logLevel)
	{
		switch (logLevel)
		{
		case LogLevel::Trace: return EB_STRING_LOG_TRACE;
		case LogLevel::Info:  return EB_STRING_LOG_INFO;
		case LogLevel::Warn:  return EB_STRING_LOG_WARN;
		case LogLevel::Error: return EB_STRING_LOG_ERROR;
		case LogLevel::Fatal: return EB_STRING_LOG_FATAL;
		default: return EB_STRING_LOG_INFO;
		}
	}

	const char* Logger::GetLogLevelColor(LogLevel logLevel)
	{
		switch (logLevel)
		{
		case LogLevel::Trace: return EB_COLOR_LOG_TRACE;
		case LogLevel::Info:  return EB_COLOR_LOG_INFO;
		case LogLevel::Warn:  return EB_COLOR_LOG_WARN;
		case LogLevel::Error: return EB_COLOR_LOG_ERROR;
		case LogLevel::Fatal: return EB_COLOR_LOG_FATAL;
		default: return EB_COLOR_LOG_INFO;
		}
	}

	const char* Logger::GetLogLevelResetColor()
	{
		return EB_COLOR_LOG_RESET;
	}

}

