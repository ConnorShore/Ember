#pragma once

#include <string>
#include <format>

namespace Ember {

	class StringUtils
	{
	public:
		static std::string GetBaseName(const std::string& name);
	};

}