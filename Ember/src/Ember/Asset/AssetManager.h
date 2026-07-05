#pragma once

#include "UUID.h"
#include "Asset.h"
#include "Model.h"
#include "Skeleton.h"
#include "PhysicsMaterial.h"
#include "Prefab.h"
#include "Font.h"
#include "AudioClip.h"
#include "NavigationMeshData.h"

#include "Serializers/MeshSerializer.h"
#include "Serializers/MaterialSerializer.h"
#include "Serializers/ModelSerializer.h"
#include "Serializers/SkeletonSerializer.h"
#include "Serializers/SkeletonMaskSerializer.h"
#include "Serializers/PhysicsMaterialSerializer.h"
#include "Serializers/NavigationMeshSerializer.h"

#include "Ember/Core/Core.h"
#include "Ember/Script/Script.h"
#include "Ember/Script/ScriptImporter.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/TextureImporter.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Animation/AnimationSerializer.h"
#include "Ember/Animation/AnimationControllerSerializer.h"

#include <unordered_map>
#include <concepts>
#include <vector>
#include <filesystem>
#include <string>

namespace Ember {

	class Scene; // forward declaration to avoid circular dependency (AssetManager.h <-> Scene.h)

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
		bool SaveAssetToFile(UUID assetUUID, const std::string& filePath)
		{
			if (!m_Assets.contains(assetUUID))
			{
				EB_CORE_ERROR("Attempted to save asset that doesn't exist!");
				return false;
			}
			auto asset = m_Assets.at(assetUUID);
			if (asset->GetType() != T::GetStaticType())
			{
				EB_CORE_ERROR("Attempted to save asset with the wrong type!");
				return false;
			}
			std::string absolutePath = std::filesystem::absolute(filePath).string();
			if constexpr (std::same_as<T, Texture2D>)
				return TextureImporter::Save(SharedPtr<Texture2D>(DynamicPointerCast<Texture2D>(asset)), absolutePath);
			else if constexpr (std::same_as<T, Shader>)
				return ShaderImporter::Save(SharedPtr<Shader>(DynamicPointerCast<Shader>(asset)), absolutePath);
			else if constexpr (std::same_as<T, Mesh>)
				return MeshSerializer::Serialize(SharedPtr<Mesh>(DynamicPointerCast<Mesh>(asset)), absolutePath);
			else if constexpr (std::same_as<T, Model>)
				return ModelSerializer::Serialize(SharedPtr<Model>(DynamicPointerCast<Model>(asset)), absolutePath);
			else if constexpr (std::same_as<T, Animation>)
				return AnimationSerializer::Serialize(SharedPtr<Animation>(DynamicPointerCast<Animation>(asset)), absolutePath);
			else if constexpr (std::same_as<T, AnimationController>)
				return AnimationControllerSerializer::Serialize(absolutePath, SharedPtr<AnimationController>(DynamicPointerCast<AnimationController>(asset)));
			else if constexpr (std::same_as<T, Skeleton>)
				return SkeletonSerializer::Serialize(SharedPtr<Skeleton>(DynamicPointerCast<Skeleton>(asset)), absolutePath);
			else if constexpr (std::same_as<T, SkeletonMask>)
				return SkeletonMaskSerializer::Serialize(SharedPtr<SkeletonMask>(DynamicPointerCast<SkeletonMask>(asset)), absolutePath);
			else if constexpr (std::same_as<T, PhysicsMaterial>)
				return PhysicsMaterialSerializer::Serialize(SharedPtr<PhysicsMaterial>(DynamicPointerCast<PhysicsMaterial>(asset)), absolutePath);
			else if constexpr (std::same_as<T, NavigationMeshData>)
				return NavigationMeshSerializer::Serialize(SharedPtr<NavigationMeshData>(DynamicPointerCast<NavigationMeshData>(asset)), absolutePath);
			else if constexpr (std::same_as<T, Scene>)
			{
				// Scenes are saved through the SceneManager, but we still want to allow them to be registered in the AssetManager
			}
			else
			{
				EB_CORE_ERROR("Attempted to save an asset type that doesn't support saving!");
				return false;
			}
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& filePath, bool engineAsset = true)
		{
			std::string fileName = std::filesystem::path(filePath).stem().string();
			return Load<T>(fileName, filePath, engineAsset);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(const std::string& name, const std::string& filePath, bool engineAsset = true)
		{
			return Load<T>(UUID(), name, filePath, engineAsset);
		}

		template<IsCoreAsset T>
		SharedPtr<T> Load(UUID uuid, const std::string& name, const std::string& filePath, bool engineAsset = true)
		{
			if (!engineAsset)
			{
				EB_CORE_INFO("Loading project asset {} ['{}'] from file: {}", (uint64_t)uuid, name, filePath);
			}

			// De-duplicate by absolute path so the same file isn't loaded twice
			auto absolutePath = std::filesystem::absolute(filePath).string();

			// Set path to relative to Assets.eba
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
			else if constexpr (std::same_as<T, Model>)
				newAsset = ModelSerializer::Deserialize(uuid, absolutePath, *this);
			else if constexpr (std::same_as<T, Script>)
				newAsset = ScriptImporter::LoadScript(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, Animation>)
				newAsset = AnimationSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::same_as<T, AnimationController>)
				newAsset = AnimationControllerSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::same_as<T, Skeleton>)
				newAsset = SkeletonSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::same_as<T, SkeletonMask>)
				newAsset = SkeletonMaskSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::same_as<T, PhysicsMaterial>)
				newAsset = PhysicsMaterialSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::same_as<T, Prefab>)
				newAsset = SharedPtr<Prefab>::Create(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, Font>)
				newAsset = SharedPtr<Font>::Create(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, AudioClip>)
				newAsset = SharedPtr<AudioClip>::Create(uuid, name, absolutePath);
			else if constexpr (std::same_as<T, NavigationMeshData>)
				newAsset = NavigationMeshSerializer::Deserialize(uuid, absolutePath);
			else if constexpr (std::derived_from<T, MaterialBase>)
			{
				auto baseMaterial = MaterialSerializer::Deserialize(uuid, absolutePath, *this);
				newAsset = DynamicPointerCast<T>(baseMaterial);
				if (!newAsset)
					EB_CORE_ERROR("Failed to load Material! The requested type did not match the file's contents.");
			}
			else if constexpr (std::same_as<T, Scene>)
			{
				// Do nothing here for loading since Scenes are loaded through the SceneManager,
				// but we still want to allow them to be registered in the AssetManager
				newAsset = SharedPtr<Scene>::Create(uuid, name, absolutePath);
			}
			else
				EB_CORE_ASSERT(false, "Attempted to call Load on a non-loadable Asset type!");

			if (!newAsset)
			{
				EB_CORE_ERROR("Failed to load asset {} ['{}'] from file: {}", (uint64_t)uuid, name, absolutePath);
				return nullptr;
			}

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

			if (!asset->GetFilePath().empty())
				m_AssetPaths[asset->GetFilePath()] = uuid;
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
		SharedPtr<T> GetAssetByPath(const std::string& path) const
		{
			auto absolutePath = std::filesystem::absolute(path).string();
			EB_CORE_ASSERT(m_AssetPaths.contains(absolutePath), "Attempted to retrieve asset that doesn't exist!");
			return DynamicPointerCast<T>(m_Assets.at(m_AssetPaths.at(absolutePath)));
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

		bool ContainsAsset(UUID id) const { return m_Assets.contains(id); }
		bool ContainsAssetWithName(const std::string& name) const { return m_AssetNames.contains(name); }
		bool ContainsAssetWithPath(const std::string& path) const { return m_AssetPaths.contains(std::filesystem::absolute(path).string()); }

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

		// Renames an asset both in-memory (Asset name + file path) and in the lookup tables.
		// `newAbsolutePath` should be the destination path *after* the file has been moved on disk.
		// Returns false if the asset does not exist or the new name/path collide with another asset.
		bool RenameAsset(UUID uuid, const std::string& newName, const std::string& newAbsolutePath)
		{
			if (!m_Assets.contains(uuid))
			{
				EB_CORE_ERROR("Attempted to rename asset that doesn't exist!");
				return false;
			}

			auto absoluteNew = std::filesystem::absolute(newAbsolutePath).string();
			auto& asset = m_Assets[uuid];

			// Reject if a different asset already owns the new name or path
			auto nameIt = m_AssetNames.find(newName);
			if (nameIt != m_AssetNames.end() && nameIt->second != uuid)
			{
				EB_CORE_ERROR("Cannot rename asset: another asset already uses the name '{}'.", newName);
				return false;
			}
			auto pathIt = m_AssetPaths.find(absoluteNew);
			if (pathIt != m_AssetPaths.end() && pathIt->second != uuid)
			{
				EB_CORE_ERROR("Cannot rename asset: another asset already uses the path '{}'.", absoluteNew);
				return false;
			}

			// Erase old entries (use whatever key the asset currently reports)
			m_AssetNames.erase(asset->GetName());
			m_AssetPaths.erase(asset->GetFilePath());

			// Update the asset itself, then reinsert under the new keys
			asset->SetName(newName);
			asset->SetFilePath(absoluteNew);
			m_AssetNames[newName] = uuid;
			m_AssetPaths[absoluteNew] = uuid;
			return true;
		}

		inline void SetEngineAssetDirectory(const std::filesystem::path& path) { m_EngineAssetDirectory = path; }
		inline const std::filesystem::path& GetEngineAssetDirectory() const { return m_EngineAssetDirectory; }

		inline void SetProjectAssetDirectory(const std::filesystem::path& path) { m_ProjectAssetDirectory = path; }
		inline const std::filesystem::path& GetProjectAssetDirectory() const { return m_ProjectAssetDirectory; }

		// Walks all non-engine Shader assets, checks the on-disk modification timestamp, and
		// reloads any shader whose source file has changed since the last poll. Engine shaders
		// are skipped because they ship with the executable and should not be edited at runtime.
		// Intended to be called once per frame from the editor's update loop.
		void PollShaderHotReload()
		{
			for (auto& [id, asset] : m_Assets)
			{
				if (asset->GetType() != Shader::GetStaticType())
					continue;
				if (asset->IsEngineAsset())
					continue;

				const std::string& path = asset->GetFilePath();
				if (path.empty())
					continue;

				std::error_code ec;
				auto stamp = std::filesystem::last_write_time(path, ec);
				if (ec)
					continue;

				auto it = m_ShaderTimestamps.find(id);
				if (it == m_ShaderTimestamps.end())
				{
					// First time we've seen this shader; record the baseline timestamp without reloading.
					m_ShaderTimestamps[id] = stamp;
					continue;
				}

				if (stamp != it->second)
				{
					it->second = stamp;
					auto shader = StaticPointerCast<Shader>(asset);
					shader->Reload();
				}
			}
		}

	private:
		std::unordered_map<UUID, SharedPtr<Asset>> m_Assets;
		std::unordered_map<std::string, UUID> m_AssetNames;
		std::unordered_map<std::string, UUID> m_AssetPaths;	// Only for Load() assets, not Create()

		// Tracks last-seen file modification timestamps for hot-reloadable shader assets.
		std::unordered_map<UUID, std::filesystem::file_time_type> m_ShaderTimestamps;

		std::filesystem::path m_EngineAssetDirectory;
		std::filesystem::path m_ProjectAssetDirectory;
	};

}