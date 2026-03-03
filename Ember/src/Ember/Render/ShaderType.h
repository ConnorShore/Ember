#pragma once

#include <string_view>

namespace Ember {

	enum class ShaderType
	{
		None = -1,
		Vertex = 0,
		Fragment
	};

	constexpr std::string_view ShaderTypeToString(ShaderType shaderType)
	{
		switch (shaderType)
		{
			case ShaderType::None: return "None";
			case ShaderType::Vertex: return "Vertex";
			case ShaderType::Fragment: return "Fragment";
			default: return "Unknown";
		}
	}
}