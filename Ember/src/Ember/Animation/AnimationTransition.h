#pragma once

#include "AnimationCondition.h"
#include "Ember/Asset/UUID.h"

#include <vector>

namespace Ember {

	struct AnimationTransition
	{
		UUID Id;
		UUID FromStateId;
		UUID ToStateId;
		// TODO: Add transition name
		float BlendDuration = 0.25f;
		std::vector<AnimationCondition> Conditions;	// List of conditions that must be met for the transition to occur
	};

}