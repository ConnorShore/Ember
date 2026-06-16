#pragma once

#include "Ember/Core/Core.h"
#include "AnimationStateMachine.h"

#include <string>

namespace Ember {

	struct AnimationLayer
	{
		std::string Name = "Base Layer";
		float Weight = 1.0f;
		UUID MaskHandle = Constants::InvalidUUID; // Points to an .ebmask asset (Invalid = Full Body)
		AnimationStateMachine StateMachine;
	};

}