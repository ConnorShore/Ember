#pragma once

#include "AnimationCondition.h"

#include <string>
#include <vector>

namespace Ember {

	struct AnimationTransition
	{
		std::string FromStateName;
		std::string ToStateName;
		float BlendDuration = 0.25f;
		std::vector<AnimationCondition> Conditions;	// List of conditions that must be met for the transition to occur
	};

}