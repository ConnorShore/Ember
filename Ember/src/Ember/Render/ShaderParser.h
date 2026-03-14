#pragma once

#include "ShaderType.h"

#include <unordered_map>

namespace Ember {

	using ShaderSourceMap = std::unordered_map<ShaderType, std::string>;
	using ShaderMacros = std::unordered_map<std::string, std::string>;

	class ShaderParser
	{
	public:
		static ShaderSourceMap Parse(const std::string& filePath, const ShaderMacros& macros = {});

	private:
		static std::string InjectMacros(const std::string& source, const ShaderMacros& macros);
		static ShaderType ShaderTypeFromLine(const std::string& type);
	};

}