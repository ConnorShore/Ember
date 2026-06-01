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

	void BoneSocketSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
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