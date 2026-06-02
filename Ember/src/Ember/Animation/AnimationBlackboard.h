#pragma once

#include <unordered_map>
#include <string>

namespace Ember {

	struct AnimationBlackboard
	{
		std::unordered_map<std::string, float> FloatParameters;
		std::unordered_map<std::string, bool> BoolParameters;

		// TODO: Other params

		void SetFloat(const std::string& name, float value)
		{
			FloatParameters[name] = value;
		}

		void SetBool(const std::string& name, bool value)
		{
			BoolParameters[name] = value;
		}
	};

}