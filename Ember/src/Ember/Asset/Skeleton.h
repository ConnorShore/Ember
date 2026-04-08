#pragma once

#include "Asset.h"

#include "Ember/Math/Math.h"

#include <string>
#include <vector>

namespace Ember {

	struct BoneTransform
	{
		Vector3f Translation;
		Quaternion Rotation;

		Matrix4f ToMatrix() const
		{
			return Math::Translate(Translation) * Math::ToMatrix4f(Rotation);
		}
	};

	struct Bone
	{
		std::string Name;
		uint32_t ParentID;
		BoneTransform LocalBindPoseTransform;
	};

	class Skeleton : public Asset
	{
	public:
		Skeleton() : Asset(Constants::InvalidUUID, "Unnamed Skeleton", "", AssetType::Skeleton) {}
		Skeleton(const std::string& name, const std::vector<Bone>& bones);
		Skeleton(const std::string& name, const std::vector<Bone>& bones, const std::vector<Matrix4f>& inverseBindTransforms);
		Skeleton(UUID uuid, const std::string& name, const std::vector<Bone>& bones, const std::vector<Matrix4f>& inverseBindTransforms);

		inline const std::vector<Bone>& GetBones() const { return m_Bones; }
		inline const std::vector<Matrix4f>& GetInverseBindTransforms() const { return m_InverseBindTransforms; }

		inline static AssetType GetStaticType() { return AssetType::Skeleton; }

	private:
		// Fallback in case the importer doesn't provide inverse bind transforms
		void CalculateInverseBindTransforms();

	private:
		std::vector<Bone> m_Bones;
		std::vector<Matrix4f> m_InverseBindTransforms;
	};

}