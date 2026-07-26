#include "ebpch.h"

#include "AnimationSerializer.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>

namespace Ember {

	namespace {
		constexpr uint32_t ANIM_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t ANIM_FILE_MAGIC = 0x414E494D; // "ANIM"
		constexpr uint32_t ANIM_FILE_VERSION = 2;

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}

		bool AnimationFileLooksBinary(const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open())
				return false;

			uint32_t magic = 0;
			file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
			return file.good() && magic == ANIM_FILE_MAGIC;
		}

		// A cooked animation is safe to use if it exists and is at least as new as its source, so
		// editing/re-importing the source (which rewrites the .ebanim) invalidates a stale cook.
		bool IsCookedAnimationFresh(const std::filesystem::path& source, const std::filesystem::path& cooked)
		{
			std::error_code ec;
			if (!std::filesystem::exists(cooked, ec) || ec)
				return false;

			auto cookedTime = std::filesystem::last_write_time(cooked, ec);
			if (ec)
				return false;

			auto sourceTime = std::filesystem::last_write_time(source, ec);
			if (ec)
				return true; // Source unreadable but the cook exists — prefer the cook over failing.

			return cookedTime >= sourceTime;
		}
	}

	bool AnimationSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation)
	{
		if (!animation)
			return false;

		auto outputPath = filepath;
		outputPath.replace_extension(".ebanim");

		ryml::Tree tree;
		auto root = tree.rootref();
		root |= ryml::MAP;

		root["Version"] << ANIM_SOURCE_FILE_VERSION;
		root["Animation"] << animation->GetName();
		root["UUID"] << static_cast<uint64_t>(animation->GetUUID());
		root["Duration"] << animation->GetDuration();

		auto tracksNode = root["Tracks"];
		tracksNode |= ryml::SEQ;
		for (const auto& track : animation->GetTracks())
		{
			auto trackNode = tracksNode.append_child();
			trackNode |= ryml::MAP;
			trackNode["BoneID"] << track.BoneID;

			auto positionsNode = trackNode["PositionKeys"];
			positionsNode |= ryml::SEQ;
			for (const auto& key : track.PositionKeyframes)
			{
				auto keyNode = positionsNode.append_child();
				keyNode |= ryml::MAP;
				keyNode["Time"] << key.TimeStamp.Seconds();
				auto valueNode = keyNode["Value"];
				valueNode |= ryml::SEQ | ryml::FLOW_SL;
				valueNode.append_child() << key.Position.x;
				valueNode.append_child() << key.Position.y;
				valueNode.append_child() << key.Position.z;
			}

			auto rotationsNode = trackNode["RotationKeys"];
			rotationsNode |= ryml::SEQ;
			for (const auto& key : track.RotationKeyframes)
			{
				auto keyNode = rotationsNode.append_child();
				keyNode |= ryml::MAP;
				keyNode["Time"] << key.TimeStamp.Seconds();
				auto valueNode = keyNode["Value"];
				valueNode |= ryml::SEQ | ryml::FLOW_SL;
				valueNode.append_child() << key.Rotation.x;
				valueNode.append_child() << key.Rotation.y;
				valueNode.append_child() << key.Rotation.z;
				valueNode.append_child() << key.Rotation.w;
			}

			auto scalesNode = trackNode["ScaleKeys"];
			scalesNode |= ryml::SEQ;
			for (const auto& key : track.ScaleKeyframes)
			{
				auto keyNode = scalesNode.append_child();
				keyNode |= ryml::MAP;
				keyNode["Time"] << key.TimeStamp.Seconds();
				auto valueNode = keyNode["Value"];
				valueNode |= ryml::SEQ | ryml::FLOW_SL;
				valueNode.append_child() << key.Scale.x;
				valueNode.append_child() << key.Scale.y;
				valueNode.append_child() << key.Scale.z;
			}
		}

		auto eventsNode = root["Events"];
		eventsNode |= ryml::SEQ;
		for (const auto& event : animation->GetEvents())
		{
			auto eventNode = eventsNode.append_child();
			eventNode |= ryml::MAP;
			eventNode["Name"] << event.Name;
			eventNode["Timestamp"] << event.Timestamp;
		}

		std::ofstream fout(outputPath);
		if (!fout.is_open())
			return false;

		fout << tree;
		fout.close();
		return true;
	}

	SharedPtr<Animation> AnimationSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open animation source file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		auto root = tree.rootref();

		float duration = 0.0f;
		if (root.has_child("Duration"))
			root["Duration"] >> duration;

		std::vector<BoneAnimationTrack> tracks;
		if (root.has_child("Tracks"))
		{
			for (auto trackNode : root["Tracks"].children())
			{
				BoneAnimationTrack track;
				if (trackNode.has_child("BoneID"))
					trackNode["BoneID"] >> track.BoneID;

				if (trackNode.has_child("PositionKeys"))
				{
					for (auto keyNode : trackNode["PositionKeys"].children())
					{
						PositionKeyframe key;
						float time = 0.0f;
						if (keyNode.has_child("Time"))
							keyNode["Time"] >> time;
						key.TimeStamp = TimeStep(time);

						auto valueNode = keyNode["Value"];
						if (valueNode.valid() && valueNode.is_seq() && valueNode.num_children() == 3)
						{
							valueNode[0] >> key.Position.x;
							valueNode[1] >> key.Position.y;
							valueNode[2] >> key.Position.z;
						}

						track.PositionKeyframes.push_back(key);
					}
				}

				if (trackNode.has_child("RotationKeys"))
				{
					for (auto keyNode : trackNode["RotationKeys"].children())
					{
						RotationKeyframe key;
						float time = 0.0f;
						if (keyNode.has_child("Time"))
							keyNode["Time"] >> time;
						key.TimeStamp = TimeStep(time);

						auto valueNode = keyNode["Value"];
						if (valueNode.valid() && valueNode.is_seq() && valueNode.num_children() == 4)
						{
							valueNode[0] >> key.Rotation.x;
							valueNode[1] >> key.Rotation.y;
							valueNode[2] >> key.Rotation.z;
							valueNode[3] >> key.Rotation.w;
						}

						track.RotationKeyframes.push_back(key);
					}
				}

				if (trackNode.has_child("ScaleKeys"))
				{
					for (auto keyNode : trackNode["ScaleKeys"].children())
					{
						ScaleKeyframe key;
						float time = 0.0f;
						if (keyNode.has_child("Time"))
							keyNode["Time"] >> time;
						key.TimeStamp = TimeStep(time);

						auto valueNode = keyNode["Value"];
						if (valueNode.valid() && valueNode.is_seq() && valueNode.num_children() == 3)
						{
							valueNode[0] >> key.Scale.x;
							valueNode[1] >> key.Scale.y;
							valueNode[2] >> key.Scale.z;
						}

						track.ScaleKeyframes.push_back(key);
					}
				}

				tracks.push_back(std::move(track));
			}
		}

		std::vector<AnimationEvent> events;
		if (root.has_child("Events"))
		{
			for (auto eventNode : root["Events"].children())
			{
				AnimationEvent evt;
				if (eventNode.has_child("Name"))
					eventNode["Name"] >> evt.Name;
				if (eventNode.has_child("Timestamp"))
					eventNode["Timestamp"] >> evt.Timestamp;
				events.push_back(std::move(evt));
			}
		}

		auto anim = SharedPtr<Animation>::Create(uuid, filepath.stem().string(), duration, tracks);
		anim->SetEvents(events);
		anim->SetFilePath(filepath.string());
		return anim;
	}

	bool AnimationSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation)
	{
		auto cookedPath = GetCookedPath(filepath);
		std::ofstream file(cookedPath, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) return false;

		// Write Header
		uint32_t magic = ANIM_FILE_MAGIC;
		uint32_t version = ANIM_FILE_VERSION;
		file.write((const char*)&magic, sizeof(uint32_t));
		file.write((const char*)&version, sizeof(uint32_t));

		// Write Metadata
		float duration = animation->GetDuration();
		file.write((const char*)&duration, sizeof(float));

		const auto& tracks = animation->GetTracks();
		uint32_t trackCount = static_cast<uint32_t>(tracks.size());
		file.write((const char*)&trackCount, sizeof(uint32_t));

		const auto& events = animation->GetEvents();
		uint32_t eventCount = static_cast<uint32_t>(events.size());
		file.write((const char*)&eventCount, sizeof(uint32_t));

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

			// Scale Keys (added in version 2)
			uint32_t scaleCount = static_cast<uint32_t>(track.ScaleKeyframes.size());
			file.write((const char*)&scaleCount, sizeof(uint32_t));
			if (scaleCount > 0)
				file.write((const char*)track.ScaleKeyframes.data(), scaleCount * sizeof(ScaleKeyframe));
		}

		// Write out animation events
		for (const auto& event : animation->GetEvents())
		{
			uint32_t nameLength = static_cast<uint32_t>(event.Name.size());
			file.write((const char*)&nameLength, sizeof(uint32_t));
			file.write(event.Name.c_str(), nameLength);
			file.write((const char*)&event.Timestamp, sizeof(float));
		}

		file.close();
		return true;
	}

	SharedPtr<Animation> AnimationSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
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

		if (version > ANIM_FILE_VERSION)
		{
			EB_CORE_ERROR("Animation file version {0} is newer than supported version {1}: {2}", version, ANIM_FILE_VERSION, filepath.string());
			return nullptr;
		}

		// 2. Read Metadata
		float duration;
		file.read((char*)&duration, sizeof(float));

		uint32_t trackCount;
		file.read((char*)&trackCount, sizeof(uint32_t));

		uint32_t eventCount;
		file.read((char*)&eventCount, sizeof(uint32_t));

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

			// Scale (added in version 2; older files leave the vector empty)
			if (version >= 2)
			{
				uint32_t scaleCount;
				file.read((char*)&scaleCount, sizeof(uint32_t));
				tracks[i].ScaleKeyframes.resize(scaleCount);
				if (scaleCount > 0)
					file.read((char*)tracks[i].ScaleKeyframes.data(), scaleCount * sizeof(ScaleKeyframe));
			}
		}

		// Read animation events
		std::vector<AnimationEvent> events(eventCount);
		for (uint32_t i = 0; i < eventCount; i++)
		{
			uint32_t nameLength;
			file.read((char*)&nameLength, sizeof(uint32_t));
			std::string eventName(nameLength, '\0');
			file.read(eventName.data(), nameLength);
			float timestamp;
			file.read((char*)&timestamp, sizeof(float));
			events[i] = { eventName, timestamp };
		}

		file.close();

		std::string name = filepath.stem().string();
		auto anim = SharedPtr<Animation>::Create(uuid, name, duration, tracks);
		anim->SetFilePath(filepath.string());
		anim->SetEvents(events);

		return anim;
	}

	bool AnimationSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation)
	{
		return SerializeSource(filepath, animation);
	}

	SharedPtr<Animation> AnimationSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			if (filepath.extension() == ".bin" || AnimationFileLooksBinary(filepath))
				return DeserializeCooked(uuid, filepath);
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			if (auto cooked = DeserializeCooked(uuid, GetCookedPath(filepath)))
				return cooked;
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::Auto:
		default:
		{
			// The registered path already points at cooked binary data.
			if (filepath.extension() == ".bin" || AnimationFileLooksBinary(filepath))
				return DeserializeCooked(uuid, filepath);

			// Source path (.ebanim): prefer a fresh cooked sibling — binary keyframe loading is far
			// faster than parsing large per-keyframe YAML.
			if (auto cookedPath = GetCookedPath(filepath); IsCookedAnimationFresh(filepath, cookedPath))
				return DeserializeCooked(uuid, cookedPath);

			// No usable cook: parse the (slow) YAML source once, then cook a binary sibling so every
			// subsequent load avoids the YAML cost entirely.
			EB_CORE_WARN("Animation '{}' has no up-to-date cooked binary; loading from YAML source (slow). "
				"Writing a cooked sibling so future loads are fast.", filepath.string());
			auto animation = DeserializeSource(uuid, filepath);
			if (animation && !SerializeCooked(filepath, animation))
				EB_CORE_WARN("Failed to write cooked animation sibling for '{}'.", filepath.string());
			return animation;
		}
		}
	}

}