#pragma once

#include "Asset.h"
#include "Skeleton.h"

#include <unordered_map>

namespace Ember {

	class SkeletonMask : public Asset
	{
	public:
		SkeletonMask();
		SkeletonMask(const std::string& name);
		SkeletonMask(UUID uuid, const std::string& name, const std::string& filePath);

		inline void SetBoneWeight(const Bone& bone, float weight) { m_BoneWeights[bone.Name] = weight; }
		inline void SetBoneWeight(const std::string& boneName, float weight) { m_BoneWeights[boneName] = weight; }
		inline float GetBoneWeight(const Bone& bone) const
		{
			auto it = m_BoneWeights.find(bone.Name);
			if (it != m_BoneWeights.end())
				return it->second;
			return 0.0f; // Default weight if bone not found
		}

		inline const std::unordered_map<std::string, float>& GetBoneWeights() const { return m_BoneWeights; }
		inline std::unordered_map<std::string, float>& GetBoneWeights() { return m_BoneWeights; }

		inline void SetSkeleton(const SharedPtr<Skeleton>& skeleton) { m_Skeleton = skeleton; }
		inline const SharedPtr<Skeleton>& GetSkeleton() const { return m_Skeleton; }

		inline static AssetType GetStaticType() { return AssetType::SkeletonMask; }

	private:
		SharedPtr<Skeleton> m_Skeleton;
		std::unordered_map<std::string, float> m_BoneWeights;
	};

}