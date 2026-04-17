#include "ebpch.h"
#include "Collision.h"

#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/Scene/Entity.h"

namespace Ember {

	bool Collision::CheckOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter /*= CollisionFilterPreset::All*/)
	{
		return CheckOverlapBoxWithData(position, rotation, scale, entity, filter).HasHit;
	}

	OverlapTestData Collision::CheckOverlapBoxWithData(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter /*= CollisionFilterPreset::All*/)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestOverlapBox(position, rotation, scale, entity, filter);
	}

	bool Collision::CheckOverlapSphere(const Vector3f& position, float radius, Entity entity, CollisionFilter filter /* = CollisionFilterPreset::All */)
	{
		return CheckOverlapSphereWithData(position, radius, entity, filter).HasHit;
	}

	OverlapTestData Collision::CheckOverlapSphereWithData(const Vector3f& position, float radius, Entity entity, CollisionFilter filter /*= CollisionFilterPreset::All*/)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestOverlapSphere(position, radius, entity, filter);
	}

	CollisionCallbackData Collision::TestCollision(Entity entity)
	{
		auto physicsSystem = Application::Instance().GetSystem<PhysicsSystem>();
		return physicsSystem->TestCollision(entity);
	}

}