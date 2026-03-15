#include "ebpch.h"
#include "AssetManager.h"

namespace Ember {

	SharedPtr<Asset> AssetManager::GetAssetBase(UUID id)
	{
		return m_Assets[id];
	}

	void AssetManager::AddAssetToMap(const SharedPtr<Asset>& asset)
	{
		m_Assets[asset->GetUUID()] = asset;
	}
	void AssetManager::AddAssetToMap(const SharedPtr<Asset>& asset, const std::string& filePath)
	{
		AddAssetToMap(asset);
		m_AssetPaths[filePath] = asset->GetUUID();
	}

}