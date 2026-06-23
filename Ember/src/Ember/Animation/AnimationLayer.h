#pragma once

#include "Ember/Core/Core.h"
#include "AnimationStateMachine.h"

#include <string>

namespace Ember {

	enum class AnimationLayerMode : uint8_t
	{
		Override = 0, // Replace the accumulated pose with this layer's output (weighted by layer Weight)
		Additive = 1, // Add this layer's pose delta on top of the accumulated pose
	};

	struct AnimationLayer
	{
		std::string Name = "Base Layer";
		float Weight = 1.0f;
		UUID MaskHandle = Constants::InvalidUUID; // Points to an .ebmask asset (Invalid = Full Body)
		AnimationLayerMode Mode = AnimationLayerMode::Override;
		AnimationStateMachine StateMachine;
	};

}