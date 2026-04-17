#include "ebpch.h"
#include "Collision.h"

#include "Ember/ECS/System/PhysicsSystem.h"

namespace Ember {

	bool Collision::CheckOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, CollisionFilter filter /*= CollisionFilterPreset::All*/)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestOverlapBox(position, rotation, scale, filter);
	}

	bool Collision::CheckOverlapSphere(const Vector3f& position, float radius, CollisionFilter filter /* = CollisionFilterPreset::All */, reactphysics3d::RigidBody* bodyToIgnore /* = nullptr */)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestOverlapSphere(position, radius, filter, bodyToIgnore);
	}

}