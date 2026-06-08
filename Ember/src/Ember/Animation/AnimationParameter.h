#pragma once

#include <cstdint>

namespace Ember {

	enum class AnimationParameterType
	{
		Float,
		Bool,
		Int,
		Trigger // A bool that resets to false once consumed
	};

	struct AnimationParameter
	{
		AnimationParameterType Type = AnimationParameterType::Float;
		float FloatValue = 0.0f;
		bool BoolValue = false;
		int32_t IntValue = 0;
	};

}
