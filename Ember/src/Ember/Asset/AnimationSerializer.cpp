#include "ebpch.h"
#include "AnimationSerializer.h"
#include <fstream>

namespace Ember {

	// Magic number for validation: "ANIM"
	const uint32_t ANIM_FILE_MAGIC = 0x414E494D;

	bool AnimationSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation)
	{
		std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) return false;

		// Write Header
		uint32_t magic = ANIM_FILE_MAGIC;
		uint32_t version = 1;
		file.write((const char*)&magic, sizeof(uint32_t));
		file.write((const char*)&version, sizeof(uint32_t));

		// Write Metadata
		float duration = animation->GetDuration();
		file.write((const char*)&duration, sizeof(float));

		const auto& tracks = animation->GetTracks();
		uint32_t trackCount = static_cast<uint32_t>(tracks.size());
		file.write((const char*)&trackCount, sizeof(uint32_t));

		// Write Bone Tracks
		for (const auto& track : tracks)
		{
			file.write((const char*)&track.BoneID, sizeof(uint32_t));

			// Position Keys
			uint32_t posCount = static_cast<uint32_t>(track.PositionKeyframes.size());
			file.write((const char*)&posCount, sizeof(uint32_t));
			if (posCount > 0)
				file.write((const char*)track.PositionKeyframes.data(), posCount * sizeof(PositionKeyframe));

			// Rotation Keys
			uint32_t rotCount = static_cast<uint32_t>(track.RotationKeyframes.size());
			file.write((const char*)&rotCount, sizeof(uint32_t));
			if (rotCount > 0)
				file.write((const char*)track.RotationKeyframes.data(), rotCount * sizeof(RotationKeyframe));

			// TODO: Scale keys
		}

		file.close();
		return true;
	}

	SharedPtr<Animation> AnimationSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to open animation file: {0}", filepath.string());
			return nullptr;
		}

		// 1. Validate Header
		uint32_t magic, version;
		file.read((char*)&magic, sizeof(uint32_t));
		file.read((char*)&version, sizeof(uint32_t));

		if (magic != ANIM_FILE_MAGIC)
		{
			EB_CORE_ERROR("Invalid animation file magic in: {0}", filepath.string());
			return nullptr;
		}

		// 2. Read Metadata
		float duration;
		file.read((char*)&duration, sizeof(float));

		uint32_t trackCount;
		file.read((char*)&trackCount, sizeof(uint32_t));

		// 3. Read Tracks
		std::vector<BoneAnimationTrack> tracks(trackCount);
		for (uint32_t i = 0; i < trackCount; i++)
		{
			file.read((char*)&tracks[i].BoneID, sizeof(uint32_t));

			// Position
			uint32_t posCount;
			file.read((char*)&posCount, sizeof(uint32_t));
			tracks[i].PositionKeyframes.resize(posCount);
			if (posCount > 0)
				file.read((char*)tracks[i].PositionKeyframes.data(), posCount * sizeof(PositionKeyframe));

			// Rotation
			uint32_t rotCount;
			file.read((char*)&rotCount, sizeof(uint32_t));
			tracks[i].RotationKeyframes.resize(rotCount);
			if (rotCount > 0)
				file.read((char*)tracks[i].RotationKeyframes.data(), rotCount * sizeof(RotationKeyframe));

			// TODO: Scale
		}

		file.close();

		std::string name = filepath.stem().string();
		auto anim = SharedPtr<Animation>::Create(name, duration, tracks);
		anim->SetFilePath(filepath.string());

		return anim;
	}

}