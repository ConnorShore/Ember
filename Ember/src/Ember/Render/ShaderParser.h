#pragma once

#include "ShaderType.h"
#include "ShaderProperties.h"

#include <unordered_map>

namespace Ember {

	using ShaderSourceMap = std::unordered_map<ShaderType, std::string>;
	using ShaderMacros = std::unordered_map<std::string, std::string>;

	struct ShaderSourceOutput
	{
		ShaderSourceMap Sources;
		std::vector<ShaderProperty> Properties;
	};

	class ShaderParser
	{
	public:
		static ShaderSourceOutput Parse(const std::string& filePath, const ShaderMacros& macros = {});

	private:
		static std::string InjectMacros(const std::string& source, const ShaderMacros& macros);
		static ShaderType ShaderTypeFromLine(const std::string& type);
	};

}