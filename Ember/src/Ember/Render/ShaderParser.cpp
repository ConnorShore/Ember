#include "ebpch.h"
#include "ShaderParser.h"

#include "Ember/Core/Core.h"

#define SHADER_TYPE_INDICATOR "#shader"

namespace Ember {

	// Parses a combined shader file that uses `#shader vertex/fragment` directives to
	// separate stages, and `// @UIProperty(...)` annotations to expose uniforms in the editor.
	ShaderSourceOutput ShaderParser::Parse(const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open shader file: {}", filePath);
			return {};
		}

		ShaderSourceMap shaderSources;
		std::vector<ShaderProperty> properties;
		std::string line;
		std::stringstream ss;
		ShaderType currentType = ShaderType::None;
		bool encounteredUIProp = false;
		ShaderProperty currentProp;
		while (getline(stream, line))
		{
			if (line.find("#shader") != std::string::npos)
			{
				if (currentType != ShaderType::None)
				{
					// Inject macros if necessary
					std::string shaderSrc = InjectMacros(ss.str(), macros);
					shaderSources[currentType] = shaderSrc;

					// Clear the stringstream for the next shader stage
					ss.str(std::string());
				}

				currentType = ShaderTypeFromLine(line);

			}
			else if (line.find("// @UIProperty(") != std::string::npos)
			{
				currentProp = {};
				encounteredUIProp = true;

				// Extract everything between the parentheses
				size_t start = line.find("(");
				size_t end = line.rfind(")");

				if (start != std::string::npos && end != std::string::npos && end > start)
				{
					std::string subLine = line.substr(start + 1, end - start - 1);
					std::stringstream propStream(subLine);
					std::string kvPair;

					// Remove whitespace helper lambda
					auto trim = [](std::string& s) {
						s.erase(0, s.find_first_not_of(" \t"));
						s.erase(s.find_last_not_of(" \t") + 1);
						};

					while (std::getline(propStream, kvPair, ','))
					{
						// find key pairs
						size_t eqPos = kvPair.find('=');
						if (eqPos == std::string::npos) 
							continue;

						std::string key = kvPair.substr(0, eqPos);
						std::string val = kvPair.substr(eqPos + 1);

						trim(key);
						trim(val);


						if (key == "Name")
						{
							// Strip the quotes from the name
							if (!val.empty() && val.front() == '"') val.erase(0, 1);
							if (!val.empty() && val.back() == '"') val.pop_back();
							currentProp.DisplayName = val;
						}
						else if (key == "Type")
						{
							if (val == "Float") currentProp.Type = ShaderPropertyType::Float;
							else if (val == "Float2") currentProp.Type = ShaderPropertyType::Float2;
							else if (val == "Float3") currentProp.Type = ShaderPropertyType::Float3;
							else if (val == "Float4") currentProp.Type = ShaderPropertyType::Float4;
							else if (val == "Color3") currentProp.Type = ShaderPropertyType::Color3;
							else if (val == "Color4") currentProp.Type = ShaderPropertyType::Color4;
							else if (val == "Slider") currentProp.Type = ShaderPropertyType::Slider;
							else if (val == "Texture") currentProp.Type = ShaderPropertyType::Texture;
							else EB_CORE_ASSERT(false, "Unknown shader property type: {}", val);
						}
						else if (key == "Min")
						{
							currentProp.Min = std::stof(val);
						}
						else if (key == "Max")
						{
							currentProp.Max = std::stof(val);
						}
						else if (key == "Step")
						{
							currentProp.Step = std::stof(val);
						}
						else if (key == "Normalize")
						{
							std::transform(val.begin(), val.end(), val.begin(), ::tolower);
							currentProp.Normalize = (val == "true" || val == "1");
						}
					}
				}
			}
			else if (encounteredUIProp)
			{
				// The line immediately after @UIProperty must be the uniform declaration;
				// parse out the uniform name and add to the property list.
				std::string_view lineView(line);
				size_t uniformPos = lineView.find("uniform");
				if (uniformPos != std::string::npos)
				{
					lineView.remove_prefix(uniformPos + strlen("uniform"));
					lineView = lineView.substr(0, lineView.find(";")); // Get everything before the semicolon
					std::string_view typeAndName = lineView;
					typeAndName.remove_prefix(typeAndName.find_first_not_of(" \t")); // Remove leading whitespace

					// The last word is the name, everything before that is the type (to support things like "vec3" or "sampler2D")
					size_t lastSpace = typeAndName.find_last_of(" \t");
					std::string_view name = typeAndName.substr(lastSpace + 1);
					currentProp.UniformName = std::string(name);
					properties.push_back(currentProp);

					// Reset for next property
					currentProp = {};
					encounteredUIProp = false;

					// Pass line to shader source as normal so the uniform is declared in the shader
					ss << line << '\n';
				}
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

		return { shaderSources, properties };
	}

	// Injects #define macros immediately after each #version directive so the
	// GLSL preprocessor sees them before any user code.
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