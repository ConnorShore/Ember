#pragma once

#include "UUID.h"
#include "Asset.h"
#include "Model.h"
#include "Skeleton.h"
#include "MeshSerializer.h"
#include "MaterialSerializer.h"
#include "ModelSerializer.h"

#include "Ember/Core/Core.h"
#include "Ember/Script/Script.h"
#include "Ember/Script/ScriptImporter.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/TextureImporter.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/Mesh.h"

#include <unordered_map>
#include <concepts>
#include <vector>
#include <filesystem>
#include <string>

namespace Ember {

	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() = default;

		void LoadDefaults();
		void ClearAssets();

		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> Create(Args&&... args)
		{
			// Abstract types (Shader, Texture) use their own factory; concrete types are constructed directly
		  SharedPtr<T> newAsset;
			if constexpr (std::is_abstract_v<T>)
			{
				newAsset = T::Create(std::forward<Args>(args)...);
			}
			else
			{
				newAsset = SharedPtr<T>::Create(std::forward<Args>(args)...);
			}

			m_Assets[newAsset->GetUUID()] = newAsset;
			m_AssetNames[newAsset->GetName()] = newAsset->GetUUID();
			return newAsset;
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& filePath)
		{
			std::string fileName = std::filesystem::path(filePath).stem().string();
			return Load<T>(fileName, filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& name, const std::string& filePath)
		{
			return Load<T>(UUID(), name, filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(UUID uuid, const std::string& name, const std::string& filePath, bool engineAsset = true)
		{
			// De-duplicate by absolute path so the same file isn't loaded twice
			auto absolutePath = std::filesystem::absolute(filePath).string();
			if (m_AssetPaths.contains(absolutePath))
			{
				return GetAsset<T>(m_AssetPaths[absolutePath]);
			}

			SharedPtr<T> newAsset;
			if constexpr (std::same_as<T, Texture2D>)
				newAsset = TextureImporter::Load(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, Shader>)
				newAsset = ShaderImporter::Load(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, Mesh>)
				newAsset = MeshSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::derived_from<T, MaterialBase>)
			{
				auto baseMaterial = MaterialSerializer::Deserialize(uuid, absolutePath, *this);
				newAsset = DynamicPointerCast<T>(baseMaterial);
				if (!newAsset)
					EB_CORE_ERROR("Failed to load Material! The requested type did not match the file's contents.");
			}
			else if constexpr (std::same_as<T, Model>)
				newAsset = ModelSerializer::Deserialize(uuid, absolutePath, *this);
			else if constexpr (std::same_as<T, Script>)
				newAsset = ScriptImporter::LoadScript(uuid, name, absolutePath);
			else
				EB_CORE_ASSERT(false, "Attempted to call Load on a non-loadable Asset type!");

			newAsset->SetIsEngineAsset(engineAsset);

			m_Assets[newAsset->GetUUID()] = newAsset;
			m_AssetNames[name] = newAsset->GetUUID();
			m_AssetPaths[absolutePath] = newAsset->GetUUID();
			return newAsset;
		}

		// Custom Loads for Shader that takes macros
		template<std::same_as<Shader> T>
		SharedPtr<T> Load(const std::string& name, const std::string& filePath, const ShaderMacros& macros)
		{
			return Load<Shader>(UUID(), name, filePath, macros);
		}

		template<std::same_as<Shader> T>
		SharedPtr<T> Load(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros)
		{
			auto absolutePath = std::filesystem::absolute(filePath).string();
			if (m_AssetPaths.contains(absolutePath))
			{
				return GetAsset<Shader>(m_AssetPaths[absolutePath]);
			}
			auto newShader = ShaderImporter::Load(uuid, name, absolutePath, macros);

			m_Assets[newShader->GetUUID()] = newShader;
			m_AssetNames[name] = newShader->GetUUID();
			m_AssetPaths[absolutePath] = newShader->GetUUID();
			return newShader;
		}

		template<IsCoreAsset T>
		void Register(UUID uuid, const SharedPtr<T>& asset)
		{
			if (!asset)
			{
				EB_CORE_WARN("Attempted to register a null asset!");
				return;
			}

			std::string name = asset->GetName();

			m_Assets[uuid] = asset;
			m_AssetNames[name] = uuid;
		}

		template<IsCoreAsset T>
		UUID Register(const SharedPtr<T>& asset)
		{
			Register(asset->GetUUID(), asset);
			return asset->GetUUID();
		}

        template<IsCoreAsset T>
		SharedPtr<T> GetAsset(UUID id) const
		{
			EB_CORE_ASSERT(m_Assets.contains(id), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets.at(id));
		}

        template<IsCoreAsset T>
		SharedPtr<T> GetAsset(const std::string& name) const
		{
			EB_CORE_ASSERT(m_AssetNames.contains(name), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets.at(m_AssetNames.at(name)));
		}

		template<IsCoreAsset T>
		std::vector<SharedPtr<T>> GetAssetsOfType() const
		{
			std::vector<SharedPtr<T>> assetsOfType;
			for (auto& [id, asset] : m_Assets)
			{
				if (asset->GetType() == T::GetStaticType())
					assetsOfType.push_back(StaticPointerCast<T>(asset));
			}
			return assetsOfType;
		}

        SharedPtr<Asset> GetAssetBase(UUID id) const;

		void RemoveAsset(const std::string& file)
		{
			auto absolutePath = std::filesystem::absolute(file).string();
			if (m_AssetPaths.contains(absolutePath)) {
				RemoveAsset(m_AssetPaths[absolutePath]);
			}
			else if (m_AssetNames.contains(absolutePath)) {
				RemoveAsset(m_AssetNames[absolutePath]);
			}
		}

		void RemoveAsset(UUID uuid)
		{
			if (m_Assets.contains(uuid)) {
				m_AssetNames.erase(m_Assets[uuid]->GetName());
				m_AssetPaths.erase(m_Assets[uuid]->GetFilePath());
				m_Assets.erase(uuid);
			}
		}
		
	private:

		// TODO: See if this ever actually gets used
		template<IsCoreAsset T>
		inline std::string GenerateName()
		{
			if constexpr (std::same_as<T, Texture2D>)
				return std::format("Texture({})", m_TextureCt++);
			else if constexpr (std::same_as<T, Shader>)
				return std::format("Shader({})", m_ShaderCt++);
			else if constexpr (std::same_as<T, Model>)
				return std::format("Model({})", m_ModelCt++);
			else if constexpr (std::same_as<T, Script>)
				return std::format("Script({})", m_ScriptCt++);
			else if constexpr (std::same_as<T, Material> || std::same_as<T, MaterialInstance>)
				return std::format("Material({})", m_MaterialCt++);

			EB_CORE_ASSERT(false, "Attempted to generate name for invalid Asset type!");
			return "";
		}

	private:
		std::unordered_map<UUID, SharedPtr<Asset>> m_Assets;
		std::unordered_map<std::string, UUID> m_AssetNames;
		std::unordered_map<std::string, UUID> m_AssetPaths;	// Only for Load() assets, not Create()
		uint32_t m_TextureCt = 0;
		uint32_t m_ShaderCt = 0;
		uint32_t m_ModelCt = 0;
		uint32_t m_MaterialCt = 0;
		uint32_t m_ScriptCt = 0;
	};

}