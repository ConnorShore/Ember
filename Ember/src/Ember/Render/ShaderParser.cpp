#include "ebpch.h"
#include "ShaderParser.h"

#include "Ember/Core/Core.h"

#define SHADER_TYPE_INDICATOR "#shader"

namespace Ember {

	std::unordered_map<ShaderType, std::string> ShaderParser::Parse(const std::string& filePath)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open shader file: {}", filePath);
			return {};
		}

		std::unordered_map<ShaderType, std::string> shaderSources;
		std::string line;
		std::stringstream ss;
		ShaderType currentType = ShaderType::None;
		while (getline(stream, line))
		{
			if (line.find("#shader") != std::string::npos)
			{
				if (currentType != ShaderType::None)
				{
					shaderSources[currentType] = ss.str();
					ss.str(std::string());
				}

				currentType = ShaderTypeFromLine(line);

			}
			else
			{
				ss << line << '\n';
			}
		}

		if (shaderSources.find(currentType) == shaderSources.end())
		{
			// Need to add this type to sources
			shaderSources[currentType] = ss.str();
		}

		return shaderSources;
	}

	ShaderType ShaderParser::ShaderTypeFromLine(const std::string& line)
	{
		// Remove shader indicator from line and any whitespace after it
		std::string type = line.substr(line.find(SHADER_TYPE_INDICATOR) + strlen(SHADER_TYPE_INDICATOR));
		type.erase(std::remove_if(type.begin(), type.end(), ::isspace), type.end());

		// To lower case
		std::transform(type.begin(), type.end(), type.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// Determine shader type
		if (type == "vertex")
			return ShaderType::Vertex;
		else if (type == "fragment" || type == "pixel")
			return ShaderType::Fragment;
		
		EB_CORE_ASSERT(false, "Failed to parse shader type from line: {}", line);
		return ShaderType::None;
	}

}