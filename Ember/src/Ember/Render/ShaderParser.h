#pragma once

#include "ShaderType.h"


namespace Ember {

	using ShaderSourceMap = std::unordered_map<ShaderType, std::string>;

	class ShaderParser
	{
	public:
		static ShaderSourceMap Parse(const std::string& filePath);
		static std::string ExtractFileName(const std::string& filePath);

	private:
		static ShaderType ShaderTypeFromLine(const std::string& type);
	};

}