#pragma once

#include "Skeleton.h"

#include "Ember/Core/Core.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {

	class AssetManager;

	class SkeletonSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Skeleton>& skeleton);
		static SharedPtr<Skeleton> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}