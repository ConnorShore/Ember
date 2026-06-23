#include "ebpch.h"
#include "SkeletonMask.h"

namespace Ember {

	SkeletonMask::SkeletonMask()
		: Asset("Unnamed Skeleton Mask", "", AssetType::SkeletonMask)
	{
	}

	SkeletonMask::SkeletonMask(const std::string& name)
		: Asset(name, "", AssetType::SkeletonMask)
	{
	}

	SkeletonMask::SkeletonMask(UUID uuid, const std::string& name, const std::string& filePath)
		: Asset(uuid, name, filePath, AssetType::SkeletonMask)
	{
	}

}