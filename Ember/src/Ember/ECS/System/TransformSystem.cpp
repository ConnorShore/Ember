#include "ebpch.h"
#include "TransformSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	TransformSystem::TransformSystem(Scene* scene)
		: m_SceneHandle(scene)
	{
	}

	void TransformSystem::OnAttach(Registry* registry)
	{
		EB_CORE_INFO("Transform System attached!");
	}

	void TransformSystem::OnDetach(Registry* registry)
	{
		EB_CORE_INFO("Transform System detached!");
	}

	void TransformSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		View view = registry->Query<RelationshipComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [relationship, transform] = registry->GetComponents<RelationshipComponent, TransformComponent>(entity);

			// Only process if its the root parent
			if (relationship.ParentHandle != Constants::Entities::InvalidEntityUUID)
				continue;

			UpdateTransformTree(entity, Matrix4f(1.0f), registry);
		}
	}

	void TransformSystem::UpdateTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Registry* registry)
	{
		auto& transform = registry->GetComponent<TransformComponent>(entity);
		auto& relationship = registry->GetComponent<RelationshipComponent>(entity);

		transform.WorldTransform = parentWorldTransform * transform.GetLocalTransform();

		for (UUID child : relationship.Children)
		{
			UpdateTransformTree(m_SceneHandle->GetEntity(child).GetEntityHandle(), transform.WorldTransform, registry);
		}
	}

}