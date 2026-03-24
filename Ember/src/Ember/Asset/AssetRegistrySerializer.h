#pragma once

#include "AssetManager.h"

#include <string>

namespace Ember {

	class AssetRegistrySerializer
	{
	public:
		AssetRegistrySerializer(AssetManager* assetManager)
			: m_AssetManagerHandle(assetManager) {
		}

		bool Serialize(const std::string& filePath);
		bool Deserialize(const std::string& filePath);

	private:
		AssetManager* m_AssetManagerHandle;
	};

}