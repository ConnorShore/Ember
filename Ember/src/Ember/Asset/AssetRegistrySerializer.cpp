#include "ebpch.h"
#include "AssetRegistrySerializer.h"

#include "Model.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/Asset/Skeleton.h" 
#include "Ember/Asset/Animation.h"

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
			}
			};

		// Serialize all standard types (ordering matters)
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Texture>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Shader>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Script>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Material>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Mesh>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Skeleton>());
		serializeType(m_AssetManagerHandle->GetAssetsOfType<Animation>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<Model>());

		serializeType(m_AssetManagerHandle->GetAssetsOfType<PhysicsMaterial>());

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
			if (type == "Texture")
				m_AssetManagerHandle->Load<Texture2D>(uuid, name, path, false);
			else if (type == "Shader")
				m_AssetManagerHandle->Load<Shader>(uuid, name, path, false);
			else if (type == "Script")
				m_AssetManagerHandle->Load<Script>(uuid, name, path, false);
			else if (type == "Mesh")
				m_AssetManagerHandle->Load<Mesh>(uuid, name, path, false);
			else if (type == "MaterialInstance" || type == "Material") {
				auto material = MaterialSerializer::Deserialize(uuid, path, *m_AssetManagerHandle);
				if (material) {
					material->SetIsEngineAsset(false);
					m_AssetManagerHandle->Register(material);
				}
			}
			else if (type == "Model")
				m_AssetManagerHandle->Load<Model>(uuid, name, path, false);
			else if (type == "Skeleton")
				m_AssetManagerHandle->Load<Skeleton>(uuid, name, path, false);
			else if (type == "Animation")
				m_AssetManagerHandle->Load<Animation>(uuid, name, path, false);
			else if (type == "PhysicsMaterial")
				m_AssetManagerHandle->Load<PhysicsMaterial>(uuid, name, path, false);
			else
				EB_CORE_WARN("Unknown asset type '{0}' in registry! Skipping.", type);
		}

		return true;
	}

}