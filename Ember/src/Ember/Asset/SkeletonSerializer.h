#pragma once

#include "Skeleton.h"
#include "AssetSerializationMode.h"

#include "Ember/Core/Core.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class SkeletonSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton);
		static SharedPtr<Skeleton> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton);
		static SharedPtr<Skeleton> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton);
		static SharedPtr<Skeleton> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}