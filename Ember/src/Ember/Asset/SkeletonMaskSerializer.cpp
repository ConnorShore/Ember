#include "ebpch.h"
#include "SkeletonMaskSerializer.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>

namespace {
	template<typename T>
	void WriteRaw(std::ofstream& stream, const T& value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template<typename T>
	bool ReadRaw(std::ifstream& stream, T& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		return stream.good();
	}

	void WriteString(std::ofstream& stream, const std::string& value)
	{
		uint16_t len = static_cast<uint16_t>(value.size());
		WriteRaw(stream, len);
		if (len > 0)
			stream.write(value.data(), len);
	}

	bool ReadString(std::ifstream& stream, std::string& value)
	{
		uint16_t len = 0;
		if (!ReadRaw(stream, len))
			return false;

		value.resize(len);
		if (len > 0)
			stream.read(value.data(), len);

		return stream.good();
	}
}

namespace Ember {
	namespace {
		constexpr uint32_t MASK_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t MASK_COOKED_MAGIC = 0x4542534D; // EBSM
		constexpr uint32_t MASK_COOKED_VERSION = 1;

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}
	}

	bool SkeletonMaskSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask)
	{
		if (!skeletonMask)
			return false;

		auto outputPath = filepath;
		outputPath.replace_extension(".ebmask");

		ryml::Tree tree;
		auto root = tree.rootref();
		root |= ryml::MAP;

		root["Version"] << MASK_SOURCE_FILE_VERSION;
		root["Mask"] << skeletonMask->GetName();
		root["UUID"] << static_cast<uint64_t>(skeletonMask->GetUUID());
		root["SkeletonHandle"] << static_cast<uint64_t>(skeletonMask->GetSkeleton() ? skeletonMask->GetSkeleton()->GetUUID() : (UUID)Constants::InvalidUUID);

		auto weightsNode = root["BoneWeights"];
		weightsNode |= ryml::SEQ;
		for (const auto& [boneName, weight] : skeletonMask->GetBoneWeights())
		{
			auto node = weightsNode.append_child();
			node |= ryml::MAP;
			node["Bone"] << boneName;
			node["Weight"] << weight;
		}

		std::ofstream fout(outputPath);
		if (!fout.is_open())
			return false;

		fout << tree;
		return true;
	}

	SharedPtr<SkeletonMask> SkeletonMaskSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
			return nullptr;

		std::stringstream ss;
		ss << stream.rdbuf();
		auto yamlData = ss.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		auto root = tree.rootref();

		auto skeletonMask = SharedPtr<SkeletonMask>::Create(uuid, filepath.stem().string(), filepath.string());
		if (root.has_child("BoneWeights"))
		{
			for (auto weightNode : root["BoneWeights"].children())
			{
				std::string boneName;
				float weight = 0.0f;
				if (weightNode.has_child("Bone")) weightNode["Bone"] >> boneName;
				if (weightNode.has_child("Weight")) weightNode["Weight"] >> weight;
				if (!boneName.empty())
					skeletonMask->SetBoneWeight(boneName, weight);
			}
		}

		return skeletonMask;
	}

	bool SkeletonMaskSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask)
	{
		if (!skeletonMask)
			return false;

		auto cookedPath = GetCookedPath(filepath);
		std::ofstream stream(cookedPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		WriteRaw(stream, MASK_COOKED_MAGIC);
		WriteRaw(stream, MASK_COOKED_VERSION);

		uint64_t skeletonHandle = skeletonMask->GetSkeleton() ? static_cast<uint64_t>(skeletonMask->GetSkeleton()->GetUUID()) : static_cast<uint64_t>(Constants::InvalidUUID);
		WriteRaw(stream, skeletonHandle);

		uint32_t count = static_cast<uint32_t>(skeletonMask->GetBoneWeights().size());
		WriteRaw(stream, count);
		for (const auto& [boneName, weight] : skeletonMask->GetBoneWeights())
		{
			WriteString(stream, boneName);
			WriteRaw(stream, weight);
		}

		return true;
	}

	SharedPtr<SkeletonMask> SkeletonMaskSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
			return nullptr;
		if (magic != MASK_COOKED_MAGIC || version > MASK_COOKED_VERSION)
			return nullptr;

		uint64_t skeletonHandle = Constants::InvalidUUID;
		ReadRaw(stream, skeletonHandle);

		auto skeletonMask = SharedPtr<SkeletonMask>::Create(uuid, filepath.stem().string(), filepath.string());

		uint32_t count = 0;
		if (!ReadRaw(stream, count))
			return skeletonMask;

		for (uint32_t i = 0; i < count; i++)
		{
			std::string boneName;
			float weight = 0.0f;
			if (!ReadString(stream, boneName) || !ReadRaw(stream, weight))
				break;
			skeletonMask->SetBoneWeight(boneName, weight);
		}

		return skeletonMask;
	}

	bool SkeletonMaskSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask)
	{
		return SerializeSource(filepath, skeletonMask);
	}

	SharedPtr<SkeletonMask> SkeletonMaskSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			return DeserializeCooked(uuid, GetCookedPath(filepath));
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
				return DeserializeCooked(uuid, filepath);
			return DeserializeSource(uuid, filepath);
		}
	}

}