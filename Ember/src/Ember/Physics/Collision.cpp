#include "ebpch.h"
#include "Collision.h"

#include "Ember/ECS/System/PhysicsSystem.h"

namespace Ember {

	bool Collision::CheckSphere(const Vector3f& position, float radius, CollisionFilter filter /* = CollisionFilterPreset::All */)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestOverlapSphere(position, radius, filter);
	}

}