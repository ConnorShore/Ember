#pragma once

#include "CollisionFilter.h"

#include "Ember/ECS/Types.h"
#include "Ember/Core/Constants.h"

namespace Ember {

	struct ColliderUserData
	{
		EntityID EntityID = Constants::Entities::InvalidEntityID;
		CollisionFilter Filter = CollisionFilterPreset::Default;
	};

}