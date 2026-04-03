#include "ebpch.h"
#include "AssetRegistrySerializer.h"

#include "Model.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

#include "Ember/Utils/SerializationUtils.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

namespace Ember {

	bool AssetRegistrySerializer::Serialize(const std::string& filePath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["AssetRegistry"];
		ryml::NodeRef assetsNode = root["Assets"];
		assetsNode |= ryml::SEQ;

		auto textures = m_AssetManagerHandle->GetAssetsOfType<Texture>();
		for (auto texture : textures) {
			if (texture->GetFilePath().empty() || texture->IsEngineAsset())
				continue;
			ryml::NodeRef textureNode = assetsNode.append_child();
			Util::SerializeGeneralAsset(textureNode, texture);
		}

		auto shaders = m_AssetManagerHandle->GetAssetsOfType<Shader>();
		for (auto shader : shaders) {
			if (shader->GetFilePath().empty() || shader->IsEngineAsset())
				continue;
			ryml::NodeRef shaderNode = assetsNode.append_child();
			Util::SerializeGeneralAsset(shaderNode, shader);
		}

		auto scripts = m_AssetManagerHandle->GetAssetsOfType<Script>();
		for (auto script : scripts) {
			if (script->GetFilePath().empty() || script->IsEngineAsset())
				continue;
			ryml::NodeRef scriptNode = assetsNode.append_child();
			Util::SerializeGeneralAsset(scriptNode, script);
		}

		auto materials = m_AssetManagerHandle->GetAssetsOfType<Material>();
		for (auto material : materials) {
			if (material->GetFilePath().empty() || material->IsEngineAsset())
				continue;
			ryml::NodeRef materialNode = assetsNode.append_child();
			Util::SerializeMaterial(materialNode, material);
		}

		auto meshes = m_AssetManagerHandle->GetAssetsOfType<Mesh>();
		for (auto mesh : meshes) {
			if (mesh->GetFilePath().empty() || mesh->IsEngineAsset())
				continue;
			ryml::NodeRef meshNode = assetsNode.append_child();
			Util::SerializeGeneralAsset(meshNode, mesh);
		}

		//auto materials = m_AssetManagerHandle->GetAssetsOfType<MaterialBase>();
		//for (auto material : materials) {
		//	if (material->IsEngineAsset())
		//		continue;
		//	ryml::NodeRef materialNode = assetsNode.append_child();
		//	Util::SerializeGeneralAsset(materialNode, material);
		//}

		auto models = m_AssetManagerHandle->GetAssetsOfType<Model>();
		for (auto model : models) {
			if (model->IsEngineAsset())
				continue;
			ryml::NodeRef modelNode = assetsNode.append_child();
			Util::SerializeGeneralAsset(modelNode, model);
		}
		
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

			// Special handling for Materials since they have a more complex structure
			if (type == "Material")
			{
				SharedPtr<MaterialBase> material = Util::DeserializeMaterial(assetNode, m_AssetManagerHandle);
				material->SetIsEngineAsset(false);

				bool instanced = false;
				assetNode["Instanced"] >> instanced;

				m_AssetManagerHandle->Register(material->GetUUID(), material);
				EB_CORE_TRACE("  Loaded Material: {0}", material->GetName());
				continue;
			}

			uint64_t uuid;
			std::string name, path;

			assetNode["UUID"] >> uuid;
			assetNode["Name"] >> name;
			assetNode["FilePath"] >> path;

			if (path.empty())
			{
				EB_CORE_WARN("Asset '{0}' has an empty file path. Skipping.", name);
				continue;
			}

			if (type == "Texture")
			{
				auto texture = m_AssetManagerHandle->Load<Texture>(uuid, name, path, false);
				EB_CORE_TRACE("Loaded Texture: {0}", name);
			}
			else if (type == "Shader")
			{
				auto shader = m_AssetManagerHandle->Load<Shader>(uuid, name, path, false);
				EB_CORE_TRACE("Loaded Shader: {0}", name);
			}
			else if (type == "Script")
			{
				auto script = m_AssetManagerHandle->Load<Script>(uuid, name, path, false);
				EB_CORE_TRACE("Loaded Script: {0}", name);
			}
			else if (type == "Mesh")
			{
				auto mesh = m_AssetManagerHandle->Load<Mesh>(uuid, name, path, false);
				EB_CORE_TRACE("Loaded Mesh: {0}", name);
			}
			else if (type == "MaterialInstance" || type == "Material")
			{
				auto material = m_AssetManagerHandle->Load<MaterialInstance>(uuid, name, path, false);
				if (material)
				{
					EB_CORE_TRACE("Loaded Material: {0}", name);
				}
			}
			else if (type == "Model")
			{
				auto model = m_AssetManagerHandle->Load<Model>(uuid, name, path, false);

				if (model)
				{
					EB_CORE_TRACE("Loaded Model: {0}", name);
				}
			}
			//else if (type == "Model")
			//{
			//	std::vector<UUID> meshUUIDs;
			//	if (assetNode.has_child("Meshes"))
			//	{
			//		for (ryml::NodeRef meshNode : assetNode["Meshes"].children())
			//		{
			//			uint64_t meshID; meshNode >> meshID;
			//			meshUUIDs.push_back(meshID);
			//		}
			//	}

			//	std::vector<UUID> materialUUIDs;
			//	if (assetNode.has_child("Materials"))
			//	{
			//		for (ryml::NodeRef matNode : assetNode["Materials"].children())
			//		{
			//			uint64_t matID; matNode >> matID;
			//			materialUUIDs.push_back(matID);
			//		}
			//	}

			//	// TODO: Update this to use .ebmodel files
			//	auto model = m_AssetManagerHandle->Load<Model>(uuid, name, path, meshUUIDs, materialUUIDs);
			//	model->SetIsEngineAsset(false);
			//	EB_CORE_TRACE("  Loaded Model: {0}", name);
			//}
		}

		return true;
	}

}