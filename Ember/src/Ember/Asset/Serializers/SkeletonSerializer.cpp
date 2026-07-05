#include "ebpch.h"
#include "SkeletonSerializer.h"
#include "Ember/Asset/SkeletonHeader.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

namespace Ember {
	namespace {
		constexpr uint32_t SKELETON_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t SKELETON_BINARY_MAGIC = 0x534B454C; // "SKEL"

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}

		bool SkeletonFileLooksBinary(const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open())
				return false;

			uint32_t magic = 0;
			file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
			return file.good() && magic == SKELETON_BINARY_MAGIC;
		}
	}

	bool SkeletonSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton)
	{
		if (!skeleton)
			return false;

		auto outputPath = filepath;
		outputPath.replace_extension(".ebskeleton");

		ryml::Tree tree;
		auto root = tree.rootref();
		root |= ryml::MAP;

		root["Version"] << SKELETON_SOURCE_FILE_VERSION;
		root["Skeleton"] << skeleton->GetName();
		root["UUID"] << static_cast<uint64_t>(skeleton->GetUUID());

		auto bonesNode = root["Bones"];
		bonesNode |= ryml::SEQ;
		for (const auto& bone : skeleton->GetBones())
		{
			auto boneNode = bonesNode.append_child();
			boneNode |= ryml::MAP;
			boneNode["Name"] << bone.Name;
			boneNode["ParentID"] << bone.ParentID;

			auto translationNode = boneNode["Translation"];
			translationNode |= ryml::SEQ | ryml::FLOW_SL;
			translationNode.append_child() << bone.LocalBindPoseTransform.Translation.x;
			translationNode.append_child() << bone.LocalBindPoseTransform.Translation.y;
			translationNode.append_child() << bone.LocalBindPoseTransform.Translation.z;

			auto rotationNode = boneNode["Rotation"];
			rotationNode |= ryml::SEQ | ryml::FLOW_SL;
			rotationNode.append_child() << bone.LocalBindPoseTransform.Rotation.x;
			rotationNode.append_child() << bone.LocalBindPoseTransform.Rotation.y;
			rotationNode.append_child() << bone.LocalBindPoseTransform.Rotation.z;
			rotationNode.append_child() << bone.LocalBindPoseTransform.Rotation.w;
		}

		auto invBindsNode = root["InverseBindTransforms"];
		invBindsNode |= ryml::SEQ;
		for (const auto& mat : skeleton->GetInverseBindTransforms())
		{
			auto matNode = invBindsNode.append_child();
			matNode |= ryml::SEQ | ryml::FLOW_SL;
			const float* ptr = reinterpret_cast<const float*>(&mat);
			for (int i = 0; i < 16; i++)
				matNode.append_child() << ptr[i];
		}

		std::ofstream fout(outputPath);
		if (!fout.is_open())
			return false;

		fout << tree;
		return true;
	}

	SharedPtr<Skeleton> SkeletonSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
			return nullptr;

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		auto root = tree.rootref();

		std::vector<Bone> bones;
		if (root.has_child("Bones"))
		{
			for (auto boneNode : root["Bones"].children())
			{
				Bone bone;
				boneNode["Name"] >> bone.Name;
				boneNode["ParentID"] >> bone.ParentID;

				if (boneNode.has_child("Translation"))
				{
					auto t = boneNode["Translation"];
					if (t.is_seq() && t.num_children() == 3)
					{
						t[0] >> bone.LocalBindPoseTransform.Translation.x;
						t[1] >> bone.LocalBindPoseTransform.Translation.y;
						t[2] >> bone.LocalBindPoseTransform.Translation.z;
					}
				}

				if (boneNode.has_child("Rotation"))
				{
					auto r = boneNode["Rotation"];
					if (r.is_seq() && r.num_children() == 4)
					{
						r[0] >> bone.LocalBindPoseTransform.Rotation.x;
						r[1] >> bone.LocalBindPoseTransform.Rotation.y;
						r[2] >> bone.LocalBindPoseTransform.Rotation.z;
						r[3] >> bone.LocalBindPoseTransform.Rotation.w;
					}
				}

				bones.push_back(std::move(bone));
			}
		}

		std::vector<Matrix4f> invBinds;
		if (root.has_child("InverseBindTransforms"))
		{
			for (auto matNode : root["InverseBindTransforms"].children())
			{
				Matrix4f mat(1.0f);
				if (matNode.is_seq() && matNode.num_children() == 16)
				{
					float* ptr = reinterpret_cast<float*>(&mat);
					for (int i = 0; i < 16; i++)
						matNode[i] >> ptr[i];
				}
				invBinds.push_back(mat);
			}
		}

		auto skeleton = SharedPtr<Skeleton>::Create(uuid, filepath.stem().string(), bones, invBinds);
		skeleton->SetFilePath(filepath.string());
		return skeleton;
	}

	bool SkeletonSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton)
	{
		auto cookedPath = GetCookedPath(filepath);
		std::ofstream file(cookedPath, std::ios::binary | std::ios::trunc);
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

	SharedPtr<Skeleton> SkeletonSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
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

	bool SkeletonSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton)
	{
		return SerializeSource(filepath, skeleton);
	}

	SharedPtr<Skeleton> SkeletonSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			if (filepath.extension() == ".bin" || SkeletonFileLooksBinary(filepath))
				return DeserializeCooked(uuid, filepath);
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			return DeserializeCooked(uuid, GetCookedPath(filepath));
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin" || SkeletonFileLooksBinary(filepath))
				return DeserializeCooked(uuid, filepath);
			return DeserializeSource(uuid, filepath);
		}
	}
}