#pragma once

#include "Ember/Math/Math.h"
#include "Ember/Physics/CollisionFilter.h"

namespace reactphysics3d { class RigidBody; }

namespace Ember {

	class Collision
	{
	public:
		static bool CheckOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, CollisionFilter filter = CollisionFilterPreset::All);
		static bool CheckOverlapSphere(const Vector3f& position, float radius, CollisionFilter filter = CollisionFilterPreset::All, reactphysics3d::RigidBody* bodyToIgnore = nullptr);
	};
}