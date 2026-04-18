#pragma once

#include "Ember/Math/Math.h"
#include "Ember/Physics/CollisionFilter.h"
#include "Ember/Physics/CollisionTestCallback.h"
#include "Ember/Physics/OverlapTestCallback.h"

namespace reactphysics3d { class RigidBody; }

namespace Ember {

	class Entity;

	class Collision
	{
	public:
		static bool CheckOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter = CollisionFilterPreset::All);
		static OverlapTestData CheckOverlapBoxWithData(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter = CollisionFilterPreset::All);
		
		static bool CheckOverlapSphere(Entity entity);
		static bool CheckOverlapSphere(const Vector3f& position, float radius, Entity entity, CollisionFilter filter = CollisionFilterPreset::All);
		static OverlapTestData CheckOverlapSphereWithData(const Vector3f& position, float radius, Entity entity, CollisionFilter filter = CollisionFilterPreset::All);
		
		static CollisionCallbackData TestCollision(Entity entity);
	};
}