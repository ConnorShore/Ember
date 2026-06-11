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
		AssetMaterial,
		AssetPhysicsMaterial,
		AssetPrefab,
		AssetFont,
		AssetAudioClip,
		AssetAnimation,
		Scene,
		SceneEntity,
		Count
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
			case DragDropPayloadType::AssetMaterial: return "ASSET_MATERIAL";
			case DragDropPayloadType::AssetPhysicsMaterial: return "ASSET_PHYSICS_MATERIAL";
			case DragDropPayloadType::AssetPrefab: return "ASSET_PREFAB";
			case DragDropPayloadType::AssetFont: return "ASSET_FONT";
			case DragDropPayloadType::AssetAudioClip: return "ASSET_AUDIO_CLIP";
			case DragDropPayloadType::AssetAnimation: return "ASSET_ANIMATION";
			case DragDropPayloadType::Scene: return "SCENE";
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
			if (str == "ASSET_MATERIAL") return DragDropPayloadType::AssetMaterial;
			if (str == "ASSET_PHYSICS_MATERIAL") return DragDropPayloadType::AssetPhysicsMaterial;
			if (str == "ASSET_PREFAB") return DragDropPayloadType::AssetPrefab;
			if (str == "ASSET_FONT") return DragDropPayloadType::AssetFont;
			if (str == "ASSET_AUDIO_CLIP") return DragDropPayloadType::AssetAudioClip;
			if (str == "ASSET_ANIMATION") return DragDropPayloadType::AssetAnimation;
			if (str == "SCENE") return DragDropPayloadType::Scene;
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
			case DragDropPayloadType::AssetMaterial: return "*.ebmat;";
			case DragDropPayloadType::AssetPhysicsMaterial: return "*.ebpmat;";
			case DragDropPayloadType::AssetScript: return "*.lua;";
			case DragDropPayloadType::AssetPrefab: return "*.ebprefab";
			case DragDropPayloadType::AssetFont: return "*.ttf;*.otf;.ebfont;";
			case DragDropPayloadType::AssetAudioClip: return "*.wav;*.mp3;*.ogg;";
			case DragDropPayloadType::AssetAnimation: return "*.ebanim;";
			case DragDropPayloadType::Scene: return "*.ebs;*.ebscene;";
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
			if (extension == ".ebmat")
				return DragDropPayloadType::AssetMaterial;
			if (extension == ".ebpmat")
				return DragDropPayloadType::AssetPhysicsMaterial;
			if (extension == ".lua")
				return DragDropPayloadType::AssetScript;
			if (extension == ".ebprefab")
				return DragDropPayloadType::AssetPrefab;
			if (extension == ".ttf" || extension == ".otf" || extension == ".ebfont")
				return DragDropPayloadType::AssetFont;
			if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
				return DragDropPayloadType::AssetAudioClip;
			if (extension == ".ebanim")
				return DragDropPayloadType::AssetAnimation;
			if (extension == ".ebs" || extension == ".ebscene")
				return DragDropPayloadType::Scene;
			return DragDropPayloadType::None;
		}
	};

}