#include "ebpch.h"
#include "ModelSerializer.h"
#include "AssetManager.h"

namespace Ember {

	SharedPtr<Model> ModelSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to open cooked model file: {0}", filepath.string());
			return nullptr;
		}

		// Read file into a string for ryml to parse
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string yamlStr = buffer.str();
		file.close();

		// Parse YAML tree
		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlStr));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("Model"))
		{
			EB_CORE_ERROR("Invalid model file format: {0}", filepath.string());
			return nullptr;
		}

		std::string modelName;
		root["Model"] >> modelName;

		ryml::NodeRef rootNodeRef = root["RootNode"];

		// We need to collect unique materials to pass to the Model constructor
		std::vector<SharedPtr<MaterialBase>> materials;
		std::unordered_map<UUID, unsigned int> materialIndexMap;

		ModelNode rootModelNode;
		DeserializeNode(rootNodeRef, rootModelNode, assetManager, materials, materialIndexMap);

		auto modelAsset = SharedPtr<Model>::Create(uuid, modelName, filepath.string(), rootModelNode, materials);
		return modelAsset;
	}

	void ModelSerializer::DeserializeNode(ryml::NodeRef yamlNode, ModelNode& modelNode, AssetManager& assetManager, std::vector<SharedPtr<MaterialBase>>& materials, std::unordered_map<UUID, unsigned int>& materialIndexMap)
	{
		yamlNode["Name"] >> modelNode.Name;

		// 1. Deserialize Transform
		if (yamlNode.has_child("Transform"))
		{
			ryml::NodeRef transformNode = yamlNode["Transform"];
			float* matPtr = (float*)&modelNode.LocalTransform;

			int i = 0;
			for (ryml::NodeRef child : transformNode.children())
			{
				child >> matPtr[i++];
			}
		}

		// 2. Deserialize Meshes
		if (yamlNode.has_child("Meshes"))
		{
			ryml::NodeRef meshesRef = yamlNode["Meshes"];
			for (ryml::NodeRef m : meshesRef.children())
			{
				uint64_t rawMeshID, rawMatID;
				m["MeshID"] >> rawMeshID;
				m["MaterialID"] >> rawMatID;

				UUID meshUUID = rawMeshID;
				UUID materialUUID = rawMatID;

				MeshMaterialNode meshNode;

				// Load the actual Mesh Asset via AssetManager!
				meshNode.MeshAsset = assetManager.GetAsset<Mesh>(meshUUID);

				// Map the UUID to a local index for this Model instance
				if (materialIndexMap.find(materialUUID) == materialIndexMap.end())
				{
					materials.push_back(assetManager.GetAsset<MaterialBase>(materialUUID));
					materialIndexMap[materialUUID] = (unsigned int)(materials.size() - 1);
				}

				meshNode.MaterialIndex = materialIndexMap[materialUUID];
				modelNode.Meshes.push_back(meshNode);
			}
		}

		// 3. Deserialize Children Recursively
		if (yamlNode.has_child("Children"))
		{
			ryml::NodeRef childrenRef = yamlNode["Children"];
			for (ryml::NodeRef c : childrenRef.children())
			{
				ModelNode childNode;
				DeserializeNode(c, childNode, assetManager, materials, materialIndexMap);
				modelNode.ChildNodes.push_back(childNode);
			}
		}
	}

}