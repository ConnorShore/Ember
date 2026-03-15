#pragma once

#include "Ember/Core/Core.h"
#include "UUID.h"
#include "Asset.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/Mesh.h"

#include <unordered_map>
#include <concepts>
#include <vector>

namespace Ember {

	template<typename T>
	concept IsCoreAsset = std::derived_from<T, Asset>;

	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() = default;

		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> Create(Args&&... args)
		{
			SharedPtr<T> newAsset = SharedPtr<T>::Create(std::forward<Args>(args)...);
			AddAssetToMap(newAsset);
			return newAsset;
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& filePath)
		{
			return Load<T>(GenerateName<T>(), filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& name, const std::string& filePath)
		{
			if (m_AssetPaths.contains(filePath))
			{
				return GetAsset<T>(m_AssetPaths[filePath]);
			}

			SharedPtr<T> newAsset;
			if constexpr (std::same_as<T, Texture>)
				newAsset = TextureImporter::Load(name, filePath);
			else if constexpr (std::same_as<T, Shader>)
				newAsset = ShaderImporter::Load(name, filePath);
			else if constexpr (std::same_as<T, Mesh>)
				newAsset = MeshImporter::Load(name, filePath);
			else
				EB_CORE_ASSERT(false, "Attempted to call Load on a non-loadable Asset type!");

			AddAssetToMap(newAsset, filePath);
			return newAsset;
		}

		template<IsCoreAsset T>
		SharedPtr<T> GetAsset(UUID id)
		{
			EB_CORE_ASSERT(m_Assets.contains(id), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets[id]);
		}

		SharedPtr<Asset> GetAssetBase(UUID id);
		
	private:
		void AddAssetToMap(const SharedPtr<Asset>& asset);
		void AddAssetToMap(const SharedPtr<Asset>& asset, const std::string& filePath);

		template<IsCoreAsset T>
		inline std::string GenerateName()
		{
			if constexpr (std::same_as<T, Texture>)
				return std::format("Texture({})", m_TextureCt++);
			else if constexpr (std::same_as<T, Shader>)
				return std::format("Shader({})", m_ShaderCt++);
			else if constexpr (std::same_as<T, Mesh>)
				return std::format("Mesh({})", m_MeshCt++);
			else if constexpr (std::same_as<T, Material> || std::same_as<T, MaterialInstance>)
				return std::format("Material({})", m_MaterialCt++);

			EB_CORE_ASSERT(false, "Attempted to generate name for invalid Asset type!");
			return "";
		}

	private:
		std::unordered_map<UUID, SharedPtr<Asset>> m_Assets;
		std::unordered_map<std::string, UUID> m_AssetPaths;	// Only for Load() assets, not Create()
		unsigned int m_TextureCt = 0;
		unsigned int m_ShaderCt = 0;
		unsigned int m_MeshCt = 0;
		unsigned int m_MaterialCt = 0;
	};

}