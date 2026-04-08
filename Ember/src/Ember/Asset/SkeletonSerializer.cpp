#include "ebpch.h"
#include "SkeletonSerializer.h"
#include "Ember/Asset/SkeletonHeader.h"
#include <fstream>

namespace Ember {

	bool SkeletonSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton)
	{
		std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			return false;

		const auto& bones = skeleton->GetBones();
		const auto& invBinds = skeleton->GetInverseBindTransforms();

		SkeletonHeader header;
		header.BoneCount = static_cast<uint32_t>(bones.size());
		file.write((char*)&header, sizeof(SkeletonHeader));

		for (const auto& bone : bones) {
			uint32_t len = static_cast<uint32_t>(bone.Name.size());
			file.write((char*)&len, sizeof(uint32_t));
			file.write(bone.Name.c_str(), len);
			file.write((char*)&bone.ParentID, sizeof(uint32_t));
			file.write((char*)&bone.LocalBindPoseTransform.Translation, sizeof(Vector3f));
			file.write((char*)&bone.LocalBindPoseTransform.Rotation, sizeof(Quaternion));
		}

		file.write((char*)invBinds.data(), invBinds.size() * sizeof(Matrix4f));
		file.close();
		return true;
	}

	SharedPtr<Skeleton> SkeletonSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
			return nullptr;

		SkeletonHeader header;
		file.read((char*)&header, sizeof(SkeletonHeader));

		std::vector<Bone> bones(header.BoneCount);
		for (uint32_t i = 0; i < header.BoneCount; i++) {
			uint32_t len; file.read((char*)&len, sizeof(uint32_t));
			bones[i].Name.resize(len);
			file.read(bones[i].Name.data(), len);
			file.read((char*)&bones[i].ParentID, sizeof(uint32_t));
			file.read((char*)&bones[i].LocalBindPoseTransform.Translation, sizeof(Vector3f));
			file.read((char*)&bones[i].LocalBindPoseTransform.Rotation, sizeof(Quaternion));
		}

		std::vector<Matrix4f> invBinds(header.BoneCount);
		file.read((char*)invBinds.data(), header.BoneCount * sizeof(Matrix4f));

		auto skeleton = SharedPtr<Skeleton>::Create(uuid, filepath.stem().string(), bones, invBinds);
		skeleton->SetFilePath(filepath.string());
		return skeleton;
	}
}