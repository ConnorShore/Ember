#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Math/Math.h"

namespace Ember {

	struct RaycastData
	{
		bool Hit = false;
		Vector3f CollisionPoint;	// The world point of the collision
		Vector3f SurfaceNormal;		// The surface normal of the collider the ray hit
		float HitFraction;			// The percent of the distant of the total ray where the collision happened
		EntityID RigidBodyEntity;	// The entity with the rigid body that was hit by the ray
		EntityID ColliderEntity;	// The entity with the collider that was hit by the ray
	};

}