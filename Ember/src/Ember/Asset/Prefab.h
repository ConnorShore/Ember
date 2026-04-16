#pragma once

#include "Asset.h"

#include <fstream>

namespace Ember {

	class Prefab : public Asset
	{
	public:
		Prefab(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::Prefab) 
		{
			// Load YAML data from file
			std::ifstream stream(filePath);
			YAMLData = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
			stream.close();
		}

		static AssetType GetStaticType() { return AssetType::Prefab; }

		// Cache yaml data
		std::string YAMLData; // TODO: Store this as binary
	};

}