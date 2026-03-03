#pragma once

#include "ShaderType.h"


namespace Ember {

	class ShaderParser
	{
	public:
		static std::unordered_map<ShaderType, std::string> Parse(const std::string& filePath);

	private:
		static ShaderType ShaderTypeFromLine(const std::string& type);
	};

}