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
		ShaderPropertyType Type = ShaderPropertyType::None;
		std::string UniformName;	// Name in shader
		std::string DisplayName;	// Name for UI
		float Min = 0.0f;
		float Max = 1.0f;
		float Step = 0.005f;
		bool Normalize = false;
	};
}