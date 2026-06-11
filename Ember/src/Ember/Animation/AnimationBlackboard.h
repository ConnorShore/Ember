#pragma once

#include "AnimationParameter.h"

#include <unordered_map>
#include <string>

namespace Ember {

	struct AnimationBlackboard
	{
		std::unordered_map<std::string, AnimationParameter> Parameters;

		void SetFloat(const std::string& name, float value)
		{
			Parameters[name] = { AnimationParameterType::Float, value, false, 0 };
		}

		void SetBool(const std::string& name, bool value)
		{
			Parameters[name] = { AnimationParameterType::Bool, 0.0f, value, 0 };
		}

		void SetInt(const std::string& name, int32_t value)
		{
			Parameters[name] = { AnimationParameterType::Int, 0.0f, false, value };
		}
	};

}