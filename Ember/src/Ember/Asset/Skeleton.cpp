#include "ebpch.h"
#include "Skeleton.h"

namespace Ember {

	// UUID = 150 for testing right now
	Skeleton::Skeleton(const std::string& name, const std::vector<Bone>& bones)
		: Asset(150, name, "", AssetType::Skeleton), m_Bones(bones)
	{
		CalculateInverseBindTransforms();
	}

	Skeleton::Skeleton(const std::string& name, const std::vector<Bone>& bones, const std::vector<Matrix4f>& inverseBindTransforms)
		: Asset(150, name, "", AssetType::Skeleton), m_Bones(bones), m_InverseBindTransforms(inverseBindTransforms)
	{
	}

	void Skeleton::CalculateInverseBindTransforms()
	{
		m_InverseBindTransforms.resize(m_Bones.size());

		// Set all the bind transforms first, then go through and invert them after
		m_InverseBindTransforms[0] = m_Bones[0].LocalBindPoseTransform.ToMatrix();
		for (size_t i = 1; i < m_Bones.size(); i++)
		{
			const Bone& bone = m_Bones[i];
			Matrix4f parentGlobalTransform = m_InverseBindTransforms[bone.ParentID];
			m_InverseBindTransforms[i] = parentGlobalTransform * bone.LocalBindPoseTransform.ToMatrix();
		}

		// Invert them
		for (size_t i = 0; i < m_InverseBindTransforms.size(); i++)
			m_InverseBindTransforms[i] = Math::Inverse(m_InverseBindTransforms[i]);
	}
}