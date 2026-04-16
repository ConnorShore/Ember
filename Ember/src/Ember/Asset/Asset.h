#pragma once

#include "Ember/Core/Core.h"

#include "UUID.h"

#include <string>

namespace Ember {

	enum class AssetType
	{
		None = -1,
		Texture,
		Mesh,
		Model,
		Skeleton,
		Animation,
		Shader,
		Material,
		Script,
		PhysicsMaterial,
		Prefab
	};



	static std::string GetAssetTypeString(AssetType type)
	{
		switch (type)
		{
		case AssetType::Texture: return "Texture";
		case AssetType::Shader: return "Shader";
		case AssetType::Mesh: return "Mesh";
		case AssetType::Model: return "Model";
		case AssetType::Material: return "Material";
		case AssetType::Script: return "Script";
		case AssetType::Animation: return "Animation";
		case AssetType::Skeleton: return "Skeleton";
		case AssetType::PhysicsMaterial: return "PhysicsMaterial";
		case AssetType::Prefab: return "Prefab";
		default: EB_CORE_ASSERT(false, "Unknown asset type!"); return "";
		}
	}

	static AssetType GetAssetTypeFromString(const std::string& typeStr)
	{
		if (typeStr == "Texture") return AssetType::Texture;
		if (typeStr == "Shader") return AssetType::Shader;
		if (typeStr == "Mesh") return AssetType::Mesh;
		if (typeStr == "Model") return AssetType::Model;
		if (typeStr == "Material") return AssetType::Material;
		if (typeStr == "Script") return AssetType::Script;
		if (typeStr == "Animation") return AssetType::Animation;
		if (typeStr == "Skeleton") return AssetType::Skeleton;
		if (typeStr == "PhysicsMaterial") return AssetType::PhysicsMaterial;
		if (typeStr == "Prefab") return AssetType::Prefab;
		
		EB_CORE_ASSERT(false, "Unknown asset type!"); 
		return AssetType::None;
	}

	class Asset : public SharedResource
	{
	public:
		Asset(const std::string& name, const std::string& filePath, AssetType type) 
			: m_UUID(UUID()), m_Type(type), m_Name(name), m_FilePath(filePath) {}

		Asset(UUID uuid, const std::string& name, const std::string& filePath, AssetType type) 
			: m_UUID(uuid), m_Type(type), m_Name(name), m_FilePath(filePath) {}

		virtual ~Asset() = default;

		inline UUID GetUUID() const { return m_UUID; }
		inline AssetType GetType() const { return m_Type; }
		inline const std::string& GetName() const { return m_Name; }
		inline void SetName(const std::string& name) { m_Name = name; }

		inline const std::string& GetFilePath() const { return m_FilePath; }
		inline void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }

		inline bool IsEngineAsset() const { return m_IsEngineAsset; }
		inline void SetIsEngineAsset(bool isEngineAsset) { m_IsEngineAsset = isEngineAsset; }

		bool operator==(const Asset& other) const { return GetUUID() == other.GetUUID(); }
		bool operator!=(const Asset& other) const { return GetUUID() != other.GetUUID(); }

		bool operator<(const Asset& other) const { return (uint64_t)GetUUID() < (uint64_t)other.GetUUID(); }
		bool operator<=(const Asset& other) const { return (uint64_t)GetUUID() <= (uint64_t)other.GetUUID(); }

		bool operator>(const Asset& other) const { return (uint64_t)GetUUID() > (uint64_t)other.GetUUID(); }
		bool operator>=(const Asset& other) const { return (uint64_t)GetUUID() >= (uint64_t)other.GetUUID(); }

	protected:
		UUID m_UUID;
		AssetType m_Type;
		std::string m_Name;
		std::string m_FilePath;
		bool m_IsEngineAsset = true;
	};

	// Concept to constrain template parameters to only be derived from Asset
	template<typename T>
	concept IsCoreAsset = std::derived_from<T, Asset>;
}