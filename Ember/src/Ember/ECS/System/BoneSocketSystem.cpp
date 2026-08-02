#include "ebpch.h"
#include "BoneSocketSystem.h"

#include "Ember/Asset/Skeleton.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	static int32_t FindBoneIndex(const std::vector<Bone>& bones, const std::string& boneName)
	{
		for (uint32_t i = 0; i < bones.size(); i++)
		{
			if (bones[i].Name == boneName)
				return static_cast<int32_t>(i);
		}

		return -1;
	}

	static Entity ResolveAnimatorEntity(Entity targetEntity, Scene* scene)
	{
		if (targetEntity.ContainsComponent<AnimatorComponent>())
			return targetEntity;

		if (!targetEntity.ContainsComponent<SkinnedMeshComponent>())
			return Entity();

		auto& skinnedMesh = targetEntity.GetComponent<SkinnedMeshComponent>();
		if (skinnedMesh.RuntimeAnimatorID != Constants::Entities::InvalidEntityID &&
			scene->GetRegistry().ContainsComponent<AnimatorComponent>(skinnedMesh.RuntimeAnimatorID))
		{
			return Entity(skinnedMesh.RuntimeAnimatorID, scene);
		}

		Entity animatorEntity = scene->GetEntity(skinnedMesh.AnimatorEntityHandle);
		if (animatorEntity != Constants::Entities::InvalidEntityID && animatorEntity.ContainsComponent<AnimatorComponent>())
		{
			skinnedMesh.RuntimeAnimatorID = animatorEntity.GetEntityHandle();
			return animatorEntity;
		}

		return Entity();
	}

	static Entity ResolveSocketBaseEntity(Entity targetEntity, Entity animatorEntity, Scene* scene)
	{
		if (targetEntity.ContainsComponent<SkinnedMeshComponent>())
			return targetEntity;

		if (animatorEntity == Constants::Entities::InvalidEntityID)
			return targetEntity;

		auto& registry = scene->GetRegistry();
		View view = registry.ActiveQuery<SkinnedMeshComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto& skinnedMesh = registry.GetComponent<SkinnedMeshComponent>(entity);
			if (skinnedMesh.AnimatorEntityHandle == animatorEntity.GetUUID() || skinnedMesh.RuntimeAnimatorID == animatorEntity.GetEntityHandle())
				return Entity(entity, scene);
		}

		return targetEntity;
	}

	void BoneSocketSystem::OnAttach()
	{
		EB_CORE_INFO("Bone Socket System attached!");
	}

	void BoneSocketSystem::OnDetach()
	{
		m_BoneNameIndices.clear();
		m_SkeletonBases.clear();

		EB_CORE_INFO("Bone Socket System detached!");
	}

	bool BoneSocketSystem::SetOffsetFromWorldTransform(BoneSocketComponent& socket, const Matrix4f& worldTransform, Scene* scene)
	{
		if (socket.TargetEntityHandle == Constants::InvalidUUID || socket.BoneName.empty())
			return false;

		Entity targetEntity = scene->GetEntity(socket.TargetEntityHandle);
		if (targetEntity == Constants::Entities::InvalidEntityID || !targetEntity.ContainsComponent<TransformComponent>())
			return false;

		Entity animatorEntity = ResolveAnimatorEntity(targetEntity, scene);
		if (animatorEntity == Constants::Entities::InvalidEntityID)
			return false;

		auto& animator = animatorEntity.GetComponent<AnimatorComponent>();
		if (animator.SkeletonHandle == Constants::InvalidUUID)
			return false;

		auto skeleton = Application::Instance().GetAssetManager().GetAsset<Skeleton>(animator.SkeletonHandle);
		if (!skeleton)
			return false;

		int32_t boneIndex = FindBoneIndex(skeleton->GetBones(), socket.BoneName);
		if (boneIndex < 0 || static_cast<size_t>(boneIndex) >= animator.BonePoseMatrices.size())
			return false;

		Entity socketBaseEntity = ResolveSocketBaseEntity(targetEntity, animatorEntity, scene);
		if (!socketBaseEntity.ContainsComponent<TransformComponent>())
			return false;

		auto& socketBaseTransform = socketBaseEntity.GetComponent<TransformComponent>();
		Matrix4f socketWorldTransform = socketBaseTransform.WorldTransform * animator.BonePoseMatrices[boneIndex];
		Matrix4f localTransform = Math::Inverse(socketWorldTransform) * worldTransform;

		Vector3f position, rotation, scale;
		if (!Math::DecomposeTransform(localTransform, position, rotation, scale))
			return false;

		socket.Position = position;
		socket.Rotation = rotation;
		socket.Scale = scale;
		socket.RuntimeBoneIndex = boneIndex;
		socket.RuntimeBoneName = socket.BoneName;
		return true;
	}

	const BoneSocketSystem::BoneNameIndexMap& BoneSocketSystem::GetBoneNameIndices(UUID skeletonHandle, const Skeleton& skeleton)
	{
		const auto& bones = skeleton.GetBones();

		BoneNameIndexCache& cache = m_BoneNameIndices[skeletonHandle];
		if (!cache.Indices.empty() && cache.BoneCount == bones.size())
			return cache.Indices;

		cache.Indices.clear();
		for (uint32_t i = 0; i < bones.size(); i++)
			cache.Indices.emplace(bones[i].Name, i);
		cache.BoneCount = bones.size();

		return cache.Indices;
	}

	// The bone entities an imported model leaves in the scene hierarchy only carry their bind-pose TRS,
	// so TransformSystem parks them (and anything parented to them, e.g. hitboxes) at the T-pose forever.
	// Push the animator's evaluated pose into them instead, matching entities to bones by name.
	void BoneSocketSystem::UpdateBoneDrivenEntities(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto& assetManager = Application::Instance().GetAssetManager();

		// Skeleton space is the skinned mesh entity's world transform — the same basis the renderer
		// skins in — so bone entities land exactly on the mesh they belong to.
		m_SkeletonBases.clear();
		View meshView = registry.ActiveQuery<SkinnedMeshComponent, TransformComponent>();
		for (EntityID meshEntity : meshView)
		{
			auto& skinnedMesh = registry.GetComponent<SkinnedMeshComponent>(meshEntity);

			EntityID animatorEntity = skinnedMesh.RuntimeAnimatorID;
			if (animatorEntity == Constants::Entities::InvalidEntityID || !registry.ContainsComponent<AnimatorComponent>(animatorEntity))
			{
				Entity resolved = scene->GetEntity(skinnedMesh.AnimatorEntityHandle);
				if (resolved == Constants::Entities::InvalidEntityID || !resolved.ContainsComponent<AnimatorComponent>())
					continue;

				animatorEntity = resolved.GetEntityHandle();
				skinnedMesh.RuntimeAnimatorID = animatorEntity;
			}

			m_SkeletonBases.emplace(animatorEntity, meshEntity);
		}

		View animatorView = registry.ActiveQuery<AnimatorComponent, TransformComponent, RelationshipComponent>();
		for (EntityID entity : animatorView)
		{
			auto& animator = registry.GetComponent<AnimatorComponent>(entity);
			if (animator.SkeletonHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			if (!skeleton || skeleton->GetBones().empty())
				continue;

			const BoneNameIndexMap& boneIndices = GetBoneNameIndices(animator.SkeletonHandle, *skeleton);
			if (boneIndices.empty())
				continue;

			auto baseIt = m_SkeletonBases.find(entity);
			EntityID skeletonBase = baseIt != m_SkeletonBases.end() ? baseIt->second : entity;
			const Matrix4f skeletonToWorld = registry.GetComponent<TransformComponent>(skeletonBase).WorldTransform;

			// Start below the animator entity: it anchors the whole character and must keep the world
			// transform TransformSystem gave it even if its name happens to collide with a bone's.
			const Matrix4f rootWorldTransform = registry.GetComponent<TransformComponent>(entity).WorldTransform;
			for (UUID child : registry.GetComponent<RelationshipComponent>(entity).Children)
			{
				Entity childEntity = scene->GetEntity(child);
				if (childEntity != Constants::Entities::InvalidEntityID)
					UpdatePosedSubtree(childEntity.GetEntityHandle(), rootWorldTransform, false, skeletonToWorld, animator, boneIndices, scene);
			}
		}
	}

	void BoneSocketSystem::UpdatePosedSubtree(EntityID entity, const Matrix4f& parentWorldTransform, bool parentPosed, const Matrix4f& skeletonToWorld,
		const AnimatorComponent& animator, const BoneNameIndexMap& boneIndices, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto& transform = registry.GetComponent<TransformComponent>(entity);
		auto& relationship = registry.GetComponent<RelationshipComponent>(entity);

		// A bone's pose matrix already accumulates its parents, so a bone entity's world transform comes
		// straight from skeleton space and doesn't depend on where it sits in the entity hierarchy.
		bool posed = false;
		if (registry.ContainsComponent<TagComponent>(entity) && !registry.ContainsComponent<SkinnedMeshComponent>(entity))
		{
			auto boneIt = boneIndices.find(registry.GetComponent<TagComponent>(entity).Tag);
			if (boneIt != boneIndices.end() && boneIt->second < animator.BonePoseMatrices.size())
			{
				transform.WorldTransform = skeletonToWorld * animator.BonePoseMatrices[boneIt->second];
				posed = true;
			}
		}

		// Non-bone entities under a posed bone (hitboxes, attached props) were composed against the
		// bind pose by TransformSystem, so they have to be rebuilt against the bone's new transform.
		if (!posed && parentPosed)
		{
			if (!relationship.IsAttachment)
			{
				transform.WorldTransform = parentWorldTransform * transform.GetLocalTransform();
			}
			else
			{
				Vector3f parentTranslation, parentRotation, parentScale;
				Math::DecomposeTransform(parentWorldTransform, parentTranslation, parentRotation, parentScale);

				Matrix4f parentTransformNoScale = Math::Translate(parentTranslation) * Math::GetRotationMatrix(parentRotation);
				transform.WorldTransform = parentTransformNoScale * transform.GetLocalTransform();
			}
		}

		for (UUID child : relationship.Children)
		{
			Entity childEntity = scene->GetEntity(child);
			if (childEntity != Constants::Entities::InvalidEntityID)
				UpdatePosedSubtree(childEntity.GetEntityHandle(), transform.WorldTransform, posed || parentPosed, skeletonToWorld, animator, boneIndices, scene);
		}
	}

	void BoneSocketSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		EB_PROFILE_FUNCTION();

		if (delta.IsZero())
			return;

		UpdateBoneDrivenEntities(scene);

		auto& registry = scene->GetRegistry();
		auto& assetManager = Application::Instance().GetAssetManager();
		View view = registry.ActiveQuery<BoneSocketComponent, TransformComponent, RelationshipComponent>();

		for (EntityID entity : view)
		{
			auto& socket = registry.GetComponent<BoneSocketComponent>(entity);
			if (socket.TargetEntityHandle == Constants::InvalidUUID || socket.BoneName.empty())
				continue;

			Entity targetEntity = scene->GetEntity(socket.TargetEntityHandle);
			if (targetEntity == Constants::Entities::InvalidEntityID || !targetEntity.ContainsComponent<TransformComponent>())
				continue;

			Entity animatorEntity = ResolveAnimatorEntity(targetEntity, scene);
			if (animatorEntity == Constants::Entities::InvalidEntityID)
				continue;

			auto& animator = animatorEntity.GetComponent<AnimatorComponent>();
			if (animator.SkeletonHandle == Constants::InvalidUUID)
				continue;

			auto skeleton = assetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
			if (!skeleton)
				continue;

			const auto& bones = skeleton->GetBones();
			if (socket.RuntimeBoneName != socket.BoneName ||
				socket.RuntimeBoneIndex < 0 ||
				static_cast<size_t>(socket.RuntimeBoneIndex) >= bones.size() ||
				bones[socket.RuntimeBoneIndex].Name != socket.BoneName)
			{
				socket.RuntimeBoneIndex = FindBoneIndex(bones, socket.BoneName);
				socket.RuntimeBoneName = socket.BoneName;
			}

			if (socket.RuntimeBoneIndex < 0 || static_cast<size_t>(socket.RuntimeBoneIndex) >= animator.BonePoseMatrices.size())
				continue;

			Entity socketBaseEntity = ResolveSocketBaseEntity(targetEntity, animatorEntity, scene);
			if (!socketBaseEntity.ContainsComponent<TransformComponent>())
				continue;

			auto& socketBaseTransform = socketBaseEntity.GetComponent<TransformComponent>();
			auto& transform = registry.GetComponent<TransformComponent>(entity);
			transform.WorldTransform = socketBaseTransform.WorldTransform * animator.BonePoseMatrices[socket.RuntimeBoneIndex] * socket.GetLocalTransform();

			auto& relationship = registry.GetComponent<RelationshipComponent>(entity);
			for (UUID child : relationship.Children)
			{
				Entity childEntity = scene->GetEntity(child);
				if (childEntity != Constants::Entities::InvalidEntityID)
					UpdateChildTransformTree(childEntity.GetEntityHandle(), transform.WorldTransform, scene);
			}
		}
	}

	void BoneSocketSystem::UpdateChildTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Scene* scene)
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
			Vector3f parentTranslation, parentRotation, parentScale;
			Math::DecomposeTransform(parentWorldTransform, parentTranslation, parentRotation, parentScale);

			Matrix4f parentTransformNoScale = Math::Translate(parentTranslation) * Math::GetRotationMatrix(parentRotation);
			transform.WorldTransform = parentTransformNoScale * transform.GetLocalTransform();
		}

		for (UUID child : relationship.Children)
		{
			Entity childEntity = scene->GetEntity(child);
			if (childEntity != Constants::Entities::InvalidEntityID)
				UpdateChildTransformTree(childEntity.GetEntityHandle(), transform.WorldTransform, scene);
		}
	}

}