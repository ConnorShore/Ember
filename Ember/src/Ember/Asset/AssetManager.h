#pragma once

#include "Ember/Core/Core.h"
#include "UUID.h"
#include "Asset.h"
#include "ModelImporter.h"
#include "Script.h"
#include "ScriptImporter.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/Mesh.h"

#include <unordered_map>
#include <concepts>
#include <vector>

namespace Ember {

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

			m_Assets[newAsset->GetUUID()] = newAsset;
			m_AssetNames[newAsset->GetName()] = newAsset->GetUUID();
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
			return Load<T>(UUID(), name, filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(UUID uuid, const std::string& name, const std::string& filePath)
		{
			if (m_AssetPaths.contains(filePath))
			{
				return GetAsset<T>(m_AssetPaths[filePath]);
			}

			SharedPtr<T> newAsset;
			if constexpr (std::same_as<T, Texture>)
				newAsset = TextureImporter::Load(uuid, name, filePath);
			else if constexpr (std::same_as<T, Shader>)
				newAsset = ShaderImporter::Load(uuid, name, filePath);
			else if constexpr (std::same_as<T, Model>)
				newAsset = ScriptImporter::Load(uuid, name, filePath, *this);
			else if constexpr (std::same_as<T, Script>)
				EB_CORE_ASSERT(false, "Scripts must be loaded with a lua state")
			else
				EB_CORE_ASSERT(false, "Attempted to call Load on a non-loadable Asset type!");

			m_Assets[newAsset->GetUUID()] = newAsset;
			m_AssetNames[name] = newAsset->GetUUID();
			m_AssetPaths[filePath] = newAsset->GetUUID();
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
			if (m_AssetPaths.contains(filePath))
			{
				return GetAsset<Shader>(m_AssetPaths[filePath]);
			}
			auto newShader = ShaderImporter::Load(uuid, name, filePath, macros);

			m_Assets[newShader->GetUUID()] = newShader;
			m_AssetNames[name] = newShader->GetUUID();
			m_AssetPaths[filePath] = newShader->GetUUID();
			return newShader;
		}

		// Custom load for model that takes mesh/material UUIDs (for deserialization)
		template<std::same_as<Model> T>
		SharedPtr<T> Load(UUID uuid, const std::string& name, const std::string& filePath, const std::vector<UUID>& meshUUIDs, const std::vector<UUID>& materialUUIDs)
		{
			auto model = ModelImporter::Load(uuid, name, filePath, *this, meshUUIDs, materialUUIDs);
			
			m_Assets[model->GetUUID()] = model;
			m_AssetNames[name] = model->GetUUID();
			m_AssetPaths[filePath] = model->GetUUID();
			return model;
		}

		// Custom load for Script that takes a sol::table
		template<std::same_as<Script> T>
		SharedPtr<T> Load(const std::string& filePath, sol::state& luaState)
		{
			auto script = ScriptImporter::LoadScript(filePath, luaState);
			m_Assets[script->GetUUID()] = script;
			m_AssetNames[script->GetName()] = script->GetUUID();
			m_AssetPaths[filePath] = script->GetUUID();
			return script;
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
		void Register(const SharedPtr<T>& asset)
		{
			Register(UUID(), asset);
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
		
	private:

		template<IsCoreAsset T>
		inline std::string GenerateName()
		{
			if constexpr (std::same_as<T, Texture>)
				return std::format("Texture({})", m_TextureCt++);
			else if constexpr (std::same_as<T, Shader>)
				return std::format("Shader({})", m_ShaderCt++);
			else if constexpr (std::same_as<T, Model>)
				return std::format("Model({})", m_ModelCt++);
			else if constexpr (std::same_as<T, Material> || std::same_as<T, MaterialInstance>)
				return std::format("Material({})", m_MaterialCt++);

			EB_CORE_ASSERT(false, "Attempted to generate name for invalid Asset type!");
			return "";
		}

	private:
		std::unordered_map<UUID, SharedPtr<Asset>> m_Assets;
		std::unordered_map<std::string, UUID> m_AssetNames;
		std::unordered_map<std::string, UUID> m_AssetPaths;	// Only for Load() assets, not Create()
		unsigned int m_TextureCt = 0;
		unsigned int m_ShaderCt = 0;
		unsigned int m_ModelCt = 0;
		unsigned int m_MaterialCt = 0;
	};

}