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

			// Start propagation from root entities; children are handled recursively.
			// A root's parent is a constant identity, so it only needs recomputing when
			// its own local TRS changes (parentChanged = false).
			if (relationship.ParentHandle != Constants::InvalidUUID)
				continue;

			UpdateTransformTree(entity, Matrix4f(1.0f), false, scene);
		}
	}

	// Recursively combines parent world transform with each entity's local transform.
	// An entity's world transform is only rebuilt when its own local TRS changed or an
	// ancestor's world transform changed this frame (parentChanged). Static subtrees then
	// pay just a few float comparisons instead of a full matrix rebuild — and, for
	// attachments, an expensive matrix decompose — every frame.
	void TransformSystem::UpdateTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, bool parentChanged, Scene* scene)
	{
		// Intentionally not profiled per-node: this fires once per entity per frame and,
		// with the tree potentially thousands of nodes deep/wide, a per-call scope generates
		// millions of trace events (multi-GB captures). The whole pass is already timed by
		// the coarse "TransformSystem::OnUpdate" scope in Scene::OnUpdateRuntime.
		Entity e(entity, scene);
		auto& transform = e.GetComponent<TransformComponent>();
		auto& relationship = e.GetComponent<RelationshipComponent>();

		const bool worldChanged = parentChanged || transform.IsLocalDirty();
		if (worldChanged)
		{
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

			// Record the TRS this WorldTransform now reflects so the entity is treated as
			// clean next frame until its local TRS changes again.
			transform.MarkWorldUpdated();
		}

		for (UUID child : relationship.Children)
		{
			UpdateTransformTree(scene->GetEntity(child).GetEntityHandle(), transform.WorldTransform, worldChanged, scene);
		}
	}

}