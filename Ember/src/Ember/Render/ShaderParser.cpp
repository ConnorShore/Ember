#include "ebpch.h"
#include "ShaderParser.h"

#include "Ember/Core/Core.h"

#define SHADER_TYPE_INDICATOR "#shader"

namespace Ember {

	ShaderSourceMap ShaderParser::Parse(const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open shader file: {}", filePath);
			return {};
		}

		ShaderSourceMap shaderSources;
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
			// Need to add this type to sources after injecting macros
			std::string shaderSrc = InjectMacros(ss.str(), macros);
			shaderSources[currentType] = shaderSrc;
		}

		return shaderSources;
	}

	std::string ShaderParser::InjectMacros(const std::string& source, const ShaderMacros& macros)
	{
		if (source.empty())
			return source;

		// Create macros
		std::stringstream ss;
		for (auto [key, value] : macros)
		{
			ss << "#define " << key << " " << value << '\n';
		}

		std::string defines = ss.str();
		ss.clear();

		// Inject the defines into the shader
		std::string result = source;
		size_t pos = 0;
		while ((pos = result.find("#version", pos)) != std::string::npos)
		{
			// Find the end of this #version line
			size_t endOfLine = result.find('\n', pos);
			if (endOfLine != std::string::npos)
			{
				// Insert the defines immediately after the newline
				result.insert(endOfLine + 1, defines);

				// Move our search position past the newly inserted block so we don't infinitely loop
				pos = endOfLine + 1 + defines.length();
			}
			else
			{
				break;
			}
		}

		return result;
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