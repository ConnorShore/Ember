#pragma once

#include <string>

namespace Ember {

	enum class DragDropPayloadType
	{
		None = 0,
		AssetTexture,
		AssetMesh,
		AssetModel,
		AssetScript,
		AssetShader,
		AssetPhysicsMaterial,
		AssetPrefab,
		SceneEntity
	};

	class DragDropUtils
	{
	public:
		static std::string DragDropPayloadTypeToString(DragDropPayloadType type)
		{
			switch (type)
			{
			case DragDropPayloadType::None: return "None";
			case DragDropPayloadType::AssetTexture: return "ASSET_TEXTURE";
			case DragDropPayloadType::AssetMesh: return "ASSET_MESH";
			case DragDropPayloadType::AssetModel: return "ASSET_MODEL";
			case DragDropPayloadType::AssetScript: return "ASSET_SCRIPT";
			case DragDropPayloadType::AssetShader: return "ASSET_SHADER";
			case DragDropPayloadType::AssetPhysicsMaterial: return "ASSET_PHYSICS_MATERIAL";
			case DragDropPayloadType::AssetPrefab: return "ASSET_PREFAB";
			case DragDropPayloadType::SceneEntity: return "SCENE_ENTITY";
			default: return "Unknown";
			}
		}

		static DragDropPayloadType StringToDragDropPayloadType(const std::string& str)
		{
			if (str == "ASSET_TEXTURE") return DragDropPayloadType::AssetTexture;
			if (str == "ASSET_MESH") return DragDropPayloadType::AssetMesh;
			if (str == "ASSET_MODEL") return DragDropPayloadType::AssetModel;
			if (str == "ASSET_SCRIPT") return DragDropPayloadType::AssetScript;
			if (str == "ASSET_SHADER") return DragDropPayloadType::AssetShader;
			if (str == "ASSET_PHYSICS_MATERIAL") return DragDropPayloadType::AssetPhysicsMaterial;
			if (str == "ASSET_PREFAB") return DragDropPayloadType::AssetPrefab;
			if (str == "SCENE_ENTITY") return DragDropPayloadType::SceneEntity;
			return DragDropPayloadType::None;
		}

		// TODO: Add mesh as distinct type and differentiate meshes/models
		static std::string DragDropPayloadTypeToExtension(DragDropPayloadType type)
		{
			switch (type)
			{
			case DragDropPayloadType::AssetTexture: return "*.png;*.jpg;*.hdr;*.jpeg;*.bmp;";
				//case DragDropPayloadType::AssetMesh: return "*.obj;*.fbx;";
			case DragDropPayloadType::AssetModel: return "*.obj;*.fbx;*.gltf;*.glb;";
			//case DragDropPayloadType::AssetMesh: return "*.ebmesh;";
			//case DragDropPayloadType::AssetModel: return "*.ebmodel;";
			case DragDropPayloadType::AssetShader: return "*.glsl;";
			case DragDropPayloadType::AssetPhysicsMaterial: return "*.ebpmat;";
			case DragDropPayloadType::AssetScript: return "*.lua;";
			case DragDropPayloadType::AssetPrefab: return "*.ebprefab";
			default: return "*.*";
			}
		}

		static DragDropPayloadType ExtensionToDragDropPayloadType(const std::string& extension)
		{
			if (extension == ".png" || extension == ".jpg" || extension == ".hdr" || extension == ".jpeg" || extension == ".bmp")
				return DragDropPayloadType::AssetTexture;
			if (extension == ".ebmesh")
				return DragDropPayloadType::AssetMesh;
			//if (extension == ".obj" || extension == ".fbx" || extension == ".gltf" || extension == ".glb")
			if (extension == ".ebmodel")
				return DragDropPayloadType::AssetModel;
			if (extension == ".glsl")
				return DragDropPayloadType::AssetShader;
			if (extension == ".ebpmat")
				return DragDropPayloadType::AssetPhysicsMaterial;
			if (extension == ".lua")
				return DragDropPayloadType::AssetScript;
			if (extension == ".ebprefab")
				return DragDropPayloadType::AssetPrefab;
			return DragDropPayloadType::None;
		}
	};

}