#pragma once

#include "System.h"
#include "Ember/ECS/Types.h"
#include "Ember/Math/Math.h"
#include "Ember/Asset/UUID.h"

#include <string>
#include <unordered_map>

namespace Ember {

	struct BoneSocketComponent;
	struct AnimatorComponent;
	class Skeleton;

	class BoneSocketSystem : public System
	{
	public:
		BoneSocketSystem() = default;
		virtual ~BoneSocketSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		static bool SetOffsetFromWorldTransform(BoneSocketComponent& socket, const Matrix4f& worldTransform, Scene* scene);

	private:
		using BoneNameIndexMap = std::unordered_map<std::string, uint32_t>;

		struct BoneNameIndexCache
		{
			size_t BoneCount = 0;
			BoneNameIndexMap Indices;
		};

		void UpdateBoneDrivenEntities(Scene* scene);
		void UpdatePosedSubtree(EntityID entity, const Matrix4f& parentWorldTransform, bool parentPosed, const Matrix4f& skeletonToWorld,
			const AnimatorComponent& animator, const BoneNameIndexMap& boneIndices, Scene* scene);
		const BoneNameIndexMap& GetBoneNameIndices(UUID skeletonHandle, const Skeleton& skeleton);

		void UpdateChildTransformTree(EntityID entity, const Matrix4f& parentWorldTransform, Scene* scene);

	private:
		// Bone name -> bone index, per skeleton. Skeletons are immutable assets, so this is built
		// once per skeleton instead of re-scanning the bone list for every entity every frame.
		std::unordered_map<UUID, BoneNameIndexCache> m_BoneNameIndices;

		// Animator entity -> the skinned mesh entity whose world transform defines skeleton space.
		// Rebuilt each frame; a member so the buckets are reused instead of reallocated per frame.
		std::unordered_map<EntityID, EntityID> m_SkeletonBases;
	};

}
