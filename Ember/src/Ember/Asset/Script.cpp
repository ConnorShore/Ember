#include "ebpch.h"
#include "Script.h"

namespace Ember {

	Script::Script(const std::string& filePath, sol::table table)
		: Script(UUID(), filePath, table)
	{

	}

	Script::Script(UUID uuid, const std::string& filePath, sol::table table)
		: Asset(uuid, std::filesystem::path(filePath).stem().string(), filePath, AssetType::Script),
		m_ClassTable(table)
	{

	}
	
}