#pragma once

#include "Ember/Asset/SkeletonMask.h"
#include "AssetSerializationMode.h"

#include <filesystem>

namespace Ember {

	class SkeletonMaskSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask);
		static SharedPtr<SkeletonMask> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask);
		static SharedPtr<SkeletonMask> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask);
		static SharedPtr<SkeletonMask> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}