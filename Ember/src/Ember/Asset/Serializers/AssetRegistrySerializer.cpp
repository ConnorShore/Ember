#include "ebpch.h"
#include "AssetRegistrySerializer.h"

#include "Ember/Asset/Model.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/Asset/Skeleton.h" 
#include "Ember/Asset/SkeletonMask.h"
#include "Ember/Asset/Prefab.h"
#include "Ember/Asset/Font.h"
#include "Ember/Asset/AudioClip.h"
#include "Ember/Asset/NavigationMeshData.h"

#include "Ember/Animation/Animation.h"
#include "Ember/Animation/AnimationController.h"

#include "Ember/Scene/Scene.h"

#include "Ember/Utils/SerializationUtils.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

namespace Ember {

	// Serializes all user-created (non-engine) assets to a YAML registry file.
	// Engine-default assets are loaded at startup and don't need persisting.
	bool AssetRegistrySerializer::Serialize(const std::string& filePath)
	{
		const std::filesystem::path ebaDir = std::filesystem::path(filePath).parent_path();

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["AssetRegistry"];
		ryml::NodeRef assetsNode = root["Assets"];
		assetsNode |= ryml::SEQ;

		auto serializeType = [&](auto assets) {
			for (auto asset : assets) {
				if (asset->GetFilePath().empty() || asset->IsEngineAsset())
					continue;

				ryml::NodeRef assetNode = assetsNode.append_child();
				Util::SerializeGeneralAsset(assetNode, asset);

				// Override FilePath with a path relative to the .eba file so the
				// registry is portable regardless of where the project is placed.
				std::string relativePath = std::filesystem::relative(
					asset->GetFilePath(), ebaDir).generic_string();
				assetNode["FilePath"] << relativePath;

				// AudioClips have no sidecar file of their own, so their load mode
				// (decode vs. stream) rides along on the registry entry.
				if (auto audioClip = DynamicPointerCast<AudioClip>(asset))
					assetNode["LoadMode"] << GetAudioLoadModeString(audioClip->GetLoadMode());
			}
		};

		// Serialize all standard types (ordering matters)
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Texture>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Shader>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Script>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Material>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Mesh>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Skeleton>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<SkeletonMask>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Animation>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<AnimationController>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Model>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<PhysicsMaterial>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<NavigationMeshData>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Prefab>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Font>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<AudioClip>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Scene>());

		// Write out to disk
		std::ofstream fout(filePath);
		fout << tree;
		fout.close();

		return true;
	}

	bool AssetRegistrySerializer::Deserialize(const std::string& filePath)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_WARN("No Asset Registry found at {0}. Starting with defaults.", filePath);
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		// Check for the "Assets" node to match your output file
		if (!root.has_child("Assets"))
		{
			EB_CORE_ERROR("Asset Registry is malformed! Missing 'Assets' node.");
			return false;
		}

		EB_CORE_INFO("Deserializing Asset Registry...");

		const std::filesystem::path ebaDir = std::filesystem::path(filePath).parent_path();

		ryml::NodeRef assetsNode = root["Assets"];
		for (ryml::NodeRef assetNode : assetsNode.children())
		{
			std::string type;
			assetNode["Type"] >> type;

			uint64_t uuid;
			std::string name, path;

			assetNode["UUID"] >> uuid;
			assetNode["Name"] >> name;
			assetNode["FilePath"] >> path;

			if (path.empty())
				continue;

			// Resolve the stored relative path to an absolute path using the
			// .eba file's directory so assets load correctly from any location.
			path = std::filesystem::weakly_canonical(ebaDir / path).string();
			if (type == "Texture")
				m_AssetManagerHandle->Load<Texture2D>(uuid, name, path, false);
			else if (type == "Shader")
				m_AssetManagerHandle->Load<Shader>(uuid, name, path, false);
			else if (type == "Script")
				m_AssetManagerHandle->Load<Script>(uuid, name, path, false);
			else if (type == "Mesh")
				m_AssetManagerHandle->Load<Mesh>(uuid, name, path, false);
			else if (type == "MaterialInstance" || type == "Material") {
				// Materials load directly (not via AssetManager::Load), so normalize to the source path
				// here too: recover it from a stale ".bin" and pin the asset's identity to it so the
				// re-saved registry stays source-based and portable.
				auto sourcePath = AssetSerializationMode::ResolveSourcePath(path).string();
				auto material = MaterialSerializer::Deserialize(uuid, sourcePath, *m_AssetManagerHandle);
				if (material) {
					material->SetIsEngineAsset(false);
					material->SetFilePath(sourcePath);
					m_AssetManagerHandle->Register(material);
				}
			}
			else if (type == "Model")
				m_AssetManagerHandle->Load<Model>(uuid, name, path, false);
			else if (type == "Skeleton")
				m_AssetManagerHandle->Load<Skeleton>(uuid, name, path, false);
			else if (type == "SkeletonMask")
				m_AssetManagerHandle->Load<SkeletonMask>(uuid, name, path, false);
			else if (type == "Animation")
				m_AssetManagerHandle->Load<Animation>(uuid, name, path, false);
			else if (type == "AnimationController")
				m_AssetManagerHandle->Load<AnimationController>(uuid, name, path, false);
			else if (type == "PhysicsMaterial")
				m_AssetManagerHandle->Load<PhysicsMaterial>(uuid, name, path, false);
			else if (type == "NavMeshData")
				m_AssetManagerHandle->Load<NavigationMeshData>(uuid, name, path, false);
			else if (type == "Prefab")
				m_AssetManagerHandle->Load<Prefab>(uuid, name, path, false);
			else if (type == "Font")
				m_AssetManagerHandle->Load<Font>(uuid, name, path, false);
			else if (type == "AudioClip")
			{
				auto audioClip = m_AssetManagerHandle->Load<AudioClip>(uuid, name, path, false);
				if (audioClip && assetNode.has_child("LoadMode"))
				{
					std::string loadMode;
					assetNode["LoadMode"] >> loadMode;
					audioClip->SetLoadMode(GetAudioLoadModeFromString(loadMode));
				}
			}
			else if (type == "Scene")
				m_AssetManagerHandle->Load<Scene>(uuid, name, path, false);
			else
				EB_CORE_WARN("Unknown asset type '{0}' in registry! Skipping.", type);
		}

		return true;
	}

}