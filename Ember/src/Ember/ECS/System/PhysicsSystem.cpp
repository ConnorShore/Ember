#include "ebpch.h"
#include "PhysicsSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {

	void PhysicsSystem::OnAttach(Registry* registry)
	{
		EB_CORE_INFO("Physics System attached!");
	}

	void PhysicsSystem::OnDetach(Registry* registry)
	{
		EB_CORE_INFO("Physics System detached!");
	}

	void PhysicsSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		View view = registry->Query<RigidBodyComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [transform, rigidBody] = registry->GetComponents<TransformComponent, RigidBodyComponent>(entity);
			transform.Transform += rigidBody.Velocity * (float)delta;
		}
	}

}