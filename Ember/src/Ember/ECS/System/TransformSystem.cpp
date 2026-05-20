#include "ebpch.h"
#include "TransformSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	void TransformSystem::OnAttach()
	{
		EB_CORE_INFO("Transform System attached!");
	}

	void TransformSystem::OnDetach()
	{
		EB_CORE_INFO("Transform System detached!");
	}

	void TransformSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		View view = scene->GetRegistry().ActiveQuery<RelationshipComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [relationship, transform] = scene->GetRegistry().GetComponents<RelationshipComponent, TransformComponent>(entity);

			// Start propagation from root entities; children are handled recursively
			if (relationship.ParentHandle != Constants::InvalidUUID)
				continue;

			UpdateTransformTree(entity, Matrix4f(1.0f), scene);
		}
	}

	// Recursively combines parent world transform with each entity's local transform
	void TransformSystem::UpdateTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto& transform = registry.GetComponent<TransformComponent>(entity);
		auto& relationship = registry.GetComponent<RelationshipComponent>(entity);

		if (!relationship.IsAttachment)
		{
			transform.WorldTransform = parentWorldTransform * transform.GetLocalTransform();
		}
		else
		{
			// Decompose parent transform to exclude scale
			Vector3f parentTranslation, parentRotation, parentScale;
			Math::DecomposeTransform(parentWorldTransform, parentTranslation, parentRotation, parentScale);

			// Rebuild parent transform without scale (translation + rotation only)
			Matrix4f parentTransformNoScale = Math::Translate(parentTranslation) * Math::GetRotationMatrix(parentRotation);

			transform.WorldTransform = parentTransformNoScale * transform.GetLocalTransform();
		}
		for (UUID child : relationship.Children)
		{
			UpdateTransformTree(scene->GetEntity(child).GetEntityHandle(), transform.WorldTransform, scene);
		}
	}

}