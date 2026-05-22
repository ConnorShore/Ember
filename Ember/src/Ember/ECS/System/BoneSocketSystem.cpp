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
		if (animatorEntity && animatorEntity.ContainsComponent<AnimatorComponent>())
		{
			skinnedMesh.RuntimeAnimatorID = animatorEntity.GetEntityHandle();
			return animatorEntity;
		}

		return Entity();
	}

	void BoneSocketSystem::OnAttach()
	{
		EB_CORE_INFO("Bone Socket System attached!");
	}

	void BoneSocketSystem::OnDetach()
	{
		EB_CORE_INFO("Bone Socket System detached!");
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
			if (!targetEntity || !targetEntity.ContainsComponent<TransformComponent>())
				continue;

			Entity animatorEntity = ResolveAnimatorEntity(targetEntity, scene);
			if (!animatorEntity)
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

			auto& targetTransform = targetEntity.GetComponent<TransformComponent>();
			auto& transform = registry.GetComponent<TransformComponent>(entity);
			transform.WorldTransform = targetTransform.WorldTransform * animator.BonePoseMatrices[socket.RuntimeBoneIndex] * socket.GetLocalTransform();

			auto& relationship = registry.GetComponent<RelationshipComponent>(entity);
			for (UUID child : relationship.Children)
			{
				Entity childEntity = scene->GetEntity(child);
				if (childEntity)
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
			if (childEntity)
				UpdateChildTransformTree(childEntity.GetEntityHandle(), transform.WorldTransform, scene);
		}
	}

}