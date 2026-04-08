#include "ebpch.h"
#include "ModelSerializer.h"
#include "AssetManager.h"

namespace Ember {

	bool ModelSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Model>& model)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Model"] << model->GetName();
		root["UUID"] << (uint64_t)model->GetUUID();
		root["SkeletonID"] << (uint64_t)model->GetSkeletonHandle();

		ryml::NodeRef rootNodeRef = root["RootNode"];
		SerializeNode(rootNodeRef, model->GetRootNode(), model->GetAllMaterials());

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();

		return true;
	}

	// Reconstructs a Model from its .ebmodel YAML manifest, loading referenced
	// mesh and material assets via the AssetManager and rebuilding the node hierarchy.
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

		uint64_t skeletonHandle;
		root["SkeletonID"] >> skeletonHandle;

		ryml::NodeRef rootNodeRef = root["RootNode"];

		// We need to collect unique materials to pass to the Model constructor
		std::vector<SharedPtr<MaterialBase>> materials;
		std::unordered_map<UUID, uint32_t> materialIndexMap;

		ModelNode rootModelNode;
		DeserializeNode(rootNodeRef, rootModelNode, assetManager, materials, materialIndexMap);

		auto modelAsset = SharedPtr<Model>::Create(uuid, modelName, filepath.string(), rootModelNode, materials, (UUID)skeletonHandle);
		return modelAsset;
	}


	void ModelSerializer::SerializeNode(ryml::NodeRef yamlNode, const ModelNode& modelNode, const std::vector<SharedPtr<MaterialBase>>& materials)
	{
		yamlNode |= ryml::MAP;
		yamlNode["Name"] << modelNode.Name;

		// 1. Serialize Transform
		ryml::NodeRef transformNode = yamlNode["Transform"];
		transformNode |= ryml::SEQ;
		const float* matPtr = (const float*)&modelNode.LocalTransform;
		for (int i = 0; i < 16; ++i)
		{
			transformNode.append_child() << matPtr[i];
		}

		// 2. Serialize Meshes
		if (!modelNode.Meshes.empty())
		{
			ryml::NodeRef meshesRef = yamlNode["Meshes"];
			meshesRef |= ryml::SEQ;
			for (const auto& meshEntry : modelNode.Meshes)
			{
				ryml::NodeRef m = meshesRef.append_child();
				m |= ryml::MAP;
				m["MeshID"] << (uint64_t)meshEntry.MeshAsset->GetUUID();

				UUID matUUID = materials[meshEntry.MaterialIndex]->GetUUID();
				m["MaterialID"] << (uint64_t)matUUID;
			}
		}

		// 3. Serialize Children
		if (!modelNode.ChildNodes.empty())
		{
			ryml::NodeRef childrenRef = yamlNode["Children"];
			childrenRef |= ryml::SEQ;
			for (const auto& child : modelNode.ChildNodes)
			{
				SerializeNode(childrenRef.append_child(), child, materials);
			}
		}
	}

	void ModelSerializer::DeserializeNode(ryml::NodeRef yamlNode, ModelNode& modelNode, AssetManager& assetManager, std::vector<SharedPtr<MaterialBase>>& materials, std::unordered_map<UUID, uint32_t>& materialIndexMap)
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

				// Deduplicate: only add each unique material UUID once to the materials list
				if (materialIndexMap.find(materialUUID) == materialIndexMap.end())
				{
					materials.push_back(assetManager.GetAsset<MaterialBase>(materialUUID));
					materialIndexMap[materialUUID] = static_cast<uint32_t>(materials.size() - 1);
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