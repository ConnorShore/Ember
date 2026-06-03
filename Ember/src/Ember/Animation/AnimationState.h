#pragma once

#include "Ember/Asset/UUID.h"
#include "Ember/Core/Constants.h"

#include <string>

namespace Ember {

	struct AnimationState
	{
		UUID Id;
		std::string Name;
		UUID AnimationHandle = Constants::InvalidUUID;
		bool Looping = false;
		float BasePlaybackSpeed = 1.0f;
	};

}