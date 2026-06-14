#include "ebpch.h"
#include "ModelSerializer.h"
#include "AssetManager.h"

namespace {
	template<typename T>
	void WriteRaw(std::ofstream& stream, const T& value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template<typename T>
	bool ReadRaw(std::ifstream& stream, T& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		return stream.good();
	}

	void WriteString(std::ofstream& stream, const std::string& value)
	{
		uint16_t len = static_cast<uint16_t>(value.size());
		WriteRaw(stream, len);
		if (len > 0)
			stream.write(value.data(), len);
	}

	bool ReadString(std::ifstream& stream, std::string& value)
	{
		uint16_t len = 0;
		if (!ReadRaw(stream, len))
			return false;

		value.resize(len);
		if (len > 0)
			stream.read(value.data(), len);

		return stream.good();
	}
}

namespace Ember {
	namespace {
		constexpr uint32_t MODEL_COOKED_MAGIC = 0x45424D44; // EBMD
		constexpr uint32_t MODEL_COOKED_VERSION = 1;

			SharedPtr<MaterialBase> ResolveModelMaterial(AssetManager& assetManager, UUID materialUUID)
			{
				if (materialUUID != Constants::InvalidUUID && assetManager.ContainsAsset(materialUUID))
					return assetManager.GetAsset<MaterialBase>(materialUUID);

				if (assetManager.ContainsAsset(Constants::Assets::StandardGeometryMatUUID))
					return assetManager.GetAsset<MaterialBase>(Constants::Assets::StandardGeometryMatUUID);

				return nullptr;
			}

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}

		void SerializeNodeCooked(std::ofstream& stream, const ModelNode& modelNode, const std::vector<SharedPtr<MaterialBase>>& materials)
		{
			WriteString(stream, modelNode.Name);

			const float* matPtr = reinterpret_cast<const float*>(&modelNode.LocalTransform);
			for (int i = 0; i < 16; i++)
				WriteRaw(stream, matPtr[i]);

			uint32_t meshCount = static_cast<uint32_t>(modelNode.Meshes.size());
			WriteRaw(stream, meshCount);
			for (const auto& meshEntry : modelNode.Meshes)
			{
				uint64_t meshID = meshEntry.MeshAsset ? static_cast<uint64_t>(meshEntry.MeshAsset->GetUUID()) : static_cast<uint64_t>(Constants::InvalidUUID);
				uint64_t materialID = static_cast<uint64_t>(materials[meshEntry.MaterialIndex]->GetUUID());
				WriteRaw(stream, meshID);
				WriteRaw(stream, materialID);
			}

			uint32_t childCount = static_cast<uint32_t>(modelNode.ChildNodes.size());
			WriteRaw(stream, childCount);
			for (const auto& child : modelNode.ChildNodes)
				SerializeNodeCooked(stream, child, materials);
		}

		bool DeserializeNodeCooked(std::ifstream& stream, ModelNode& modelNode, AssetManager& assetManager, std::vector<SharedPtr<MaterialBase>>& materials, std::unordered_map<UUID, uint32_t>& materialIndexMap)
		{
			if (!ReadString(stream, modelNode.Name))
				return false;

			float* matPtr = reinterpret_cast<float*>(&modelNode.LocalTransform);
			for (int i = 0; i < 16; i++)
			{
				if (!ReadRaw(stream, matPtr[i]))
					return false;
			}

			uint32_t meshCount = 0;
			if (!ReadRaw(stream, meshCount))
				return false;

			for (uint32_t meshIndex = 0; meshIndex < meshCount; meshIndex++)
			{
				uint64_t meshID = Constants::InvalidUUID;
				uint64_t materialID = Constants::InvalidUUID;
				if (!ReadRaw(stream, meshID) || !ReadRaw(stream, materialID))
					return false;

				MeshMaterialNode meshNode;
				if (meshID != Constants::InvalidUUID && assetManager.ContainsAsset(UUID(meshID)))
					meshNode.MeshAsset = assetManager.GetAsset<Mesh>(UUID(meshID));

				UUID materialUUID = UUID(materialID);
				if (!materialIndexMap.contains(materialUUID))
				{
					materials.push_back(ResolveModelMaterial(assetManager, materialUUID));
					materialIndexMap[materialUUID] = static_cast<uint32_t>(materials.size() - 1);
				}

				meshNode.MaterialIndex = materialIndexMap[materialUUID];
				modelNode.Meshes.push_back(meshNode);
			}

			uint32_t childCount = 0;
			if (!ReadRaw(stream, childCount))
				return false;

			modelNode.ChildNodes.resize(childCount);
			for (uint32_t childIndex = 0; childIndex < childCount; childIndex++)
			{
				if (!DeserializeNodeCooked(stream, modelNode.ChildNodes[childIndex], assetManager, materials, materialIndexMap))
					return false;
			}

			return true;
		}
	}

	bool ModelSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Model>& model)
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
	SharedPtr<Model> ModelSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
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

	bool ModelSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Model>& model)
	{
		if (!model)
			return false;

		auto cookedPath = GetCookedPath(filepath);
		std::ofstream stream(cookedPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		WriteRaw(stream, MODEL_COOKED_MAGIC);
		WriteRaw(stream, MODEL_COOKED_VERSION);
		WriteString(stream, model->GetName());
		WriteRaw(stream, static_cast<uint64_t>(model->GetSkeletonHandle()));
		SerializeNodeCooked(stream, model->GetRootNode(), model->GetAllMaterials());
		return true;
	}

	SharedPtr<Model> ModelSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
			return nullptr;
		if (magic != MODEL_COOKED_MAGIC || version > MODEL_COOKED_VERSION)
			return nullptr;

		std::string modelName;
		if (!ReadString(stream, modelName))
			return nullptr;

		uint64_t skeletonHandle = Constants::InvalidUUID;
		if (!ReadRaw(stream, skeletonHandle))
			return nullptr;

		std::vector<SharedPtr<MaterialBase>> materials;
		std::unordered_map<UUID, uint32_t> materialIndexMap;
		ModelNode rootModelNode;
		if (!DeserializeNodeCooked(stream, rootModelNode, assetManager, materials, materialIndexMap))
			return nullptr;

		auto modelAsset = SharedPtr<Model>::Create(uuid, modelName, filepath.string(), rootModelNode, materials, UUID(skeletonHandle));
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
				if (assetManager.ContainsAsset(meshUUID))
					meshNode.MeshAsset = assetManager.GetAsset<Mesh>(meshUUID);

				// Deduplicate: only add each unique material UUID once to the materials list
				if (materialIndexMap.find(materialUUID) == materialIndexMap.end())
				{
					materials.push_back(ResolveModelMaterial(assetManager, materialUUID));
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

	bool ModelSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<Model>& model)
	{
		return SerializeSource(filepath, model);
	}

	SharedPtr<Model> ModelSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath, AssetManager& assetManager)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath, assetManager);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			return DeserializeCooked(uuid, GetCookedPath(filepath), assetManager);
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
				return DeserializeCooked(uuid, filepath, assetManager);
			return DeserializeSource(uuid, filepath, assetManager);
		}
	}

}