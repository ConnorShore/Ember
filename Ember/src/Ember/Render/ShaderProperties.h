#pragma once

#include <string>

namespace Ember {

	// Defined in shader like: "// @UIProperty("My Property", Float3)"
	enum class ShaderPropertyType
	{
		None = 0,
		Float,
		Float2,
		Float3,
		Float4,
		Color3,
		Color4,
		Slider
	};

	struct ShaderProperty
	{
		std::string UniformName;	// Name in shader
		std::string DisplayName;	// Name for UI
		ShaderPropertyType Type = ShaderPropertyType::None;
	};
}