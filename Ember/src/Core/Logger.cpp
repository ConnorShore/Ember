#include "ebpch.h"
#include "Logger.h"

#include <print>

namespace Ember {

	void Logger::Log(const std::string& message)
	{
		std::println("INFO: {}", message);
	}

}

