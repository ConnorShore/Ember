#include "ebpch.h"
#include "PhysicsSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	void PhysicsSystem::OnAttach()
	{
		EB_CORE_INFO("Physics System attached!");
	}

	void PhysicsSystem::OnDetach()
	{
		EB_CORE_INFO("Physics System detached!");
	}

	void PhysicsSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		View view = scene->GetRegistry().Query<RigidBodyComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [transform, rigidBody] = scene->GetRegistry().GetComponents<TransformComponent, RigidBodyComponent>(entity);
			transform.Position += rigidBody.Velocity * (float)delta;
		}
	}

}