#include "ebpch.h"
#include "Logger.h"

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

