#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Filter.h"
#include "Ember/Core/Constants.h"

namespace Ember {

	struct ColliderUserData
	{
		EntityID EntityID = Constants::Entities::InvalidEntityID;
		Filter Filter = FilterPreset::Default;
	};

}