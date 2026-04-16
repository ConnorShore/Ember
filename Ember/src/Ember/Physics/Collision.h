#pragma once

#include "Ember/Math/Math.h"
#include "Ember/Physics/CollisionFilter.h"

namespace Ember {

	class Collision
	{
	public:
		static bool CheckSphere(const Vector3f& position, float radius, CollisionFilter filter = CollisionFilterPreset::All);
	};
}