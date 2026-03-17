#pragma once

#include "Ember/Core/Core.h"
#include "UUID.h"
#include "Asset.h"
#include "ModelImporter.h"
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

		void LoadDefaults();

		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> Create(Args&&... args)
		{
          SharedPtr<T> newAsset;
			if constexpr (std::is_abstract_v<T>)
			{
				newAsset = T::Create(std::forward<Args>(args)...);
			}
			else
			{
				newAsset = SharedPtr<T>::Create(std::forward<Args>(args)...);
			}
			AddCreateAssetToMap(newAsset, newAsset->GetName());
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
			else if constexpr (std::same_as<T, Model>)
				newAsset = ModelImporter::Load(name, filePath, *this);
			//else if constexpr (std::same_as<T, Mesh>)
			//	newAsset = MeshImporter::Load(name, filePath);
			else
				EB_CORE_ASSERT(false, "Attempted to call Load on a non-loadable Asset type!");

			AddLoadAssetToMap(newAsset, filePath);
			return newAsset;
		}

		template<IsCoreAsset T>
		SharedPtr<T> GetAsset(UUID id)
		{
			EB_CORE_ASSERT(m_Assets.contains(id), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets[id]);
		}

		template<IsCoreAsset T>
		SharedPtr<T> GetAsset(const std::string& name)
		{
			EB_CORE_ASSERT(m_AssetNames.contains(name), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets[m_AssetNames[name]]);
		}

		SharedPtr<Asset> GetAssetBase(UUID id);
		
	private:
		void AddCreateAssetToMap(const SharedPtr<Asset>& asset, const std::string& name);
		void AddLoadAssetToMap(const SharedPtr<Asset>& asset, const std::string& filePath);

		template<IsCoreAsset T>
		inline std::string GenerateName()
		{
			if constexpr (std::same_as<T, Texture>)
				return std::format("Texture({})", m_TextureCt++);
			else if constexpr (std::same_as<T, Shader>)
				return std::format("Shader({})", m_ShaderCt++);
			else if constexpr (std::same_as<T, Model>)
				return std::format("Model({})", m_ModelCt++);
			//else if constexpr (std::same_as<T, Mesh>)
			//	return std::format("Mesh({})", m_MeshCt++);
			else if constexpr (std::same_as<T, Material> || std::same_as<T, MaterialInstance>)
				return std::format("Material({})", m_MaterialCt++);

			EB_CORE_ASSERT(false, "Attempted to generate name for invalid Asset type!");
			return "";
		}

	private:
		std::unordered_map<UUID, SharedPtr<Asset>> m_Assets;
		std::unordered_map<std::string, UUID> m_AssetPaths;	// Only for Load() assets, not Create()
		std::unordered_map<std::string, UUID> m_AssetNames;	// Only for Create() assets, not Load()
		unsigned int m_TextureCt = 0;
		unsigned int m_ShaderCt = 0;
		unsigned int m_ModelCt = 0;
		//unsigned int m_MeshCt = 0;
		unsigned int m_MaterialCt = 0;
	};

}