#include "ebpch.h"
#include "Script.h"

namespace Ember {

	Script::Script(const std::string& filePath)
		: Script(UUID(), std::filesystem::path(filePath).stem().string(), filePath)
	{

	}

	Script::Script(UUID uuid, const std::string& name, const std::string& filePath)
		: Asset(uuid, name, filePath, AssetType::Script)
	{

	}
	
}