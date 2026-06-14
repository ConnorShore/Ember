#pragma once

#include "SkeletonMask.h"

#include <filesystem>

namespace Ember {

	class SkeletonMaskSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<SkeletonMask>& skeletonMask);
		static SharedPtr<SkeletonMask> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}