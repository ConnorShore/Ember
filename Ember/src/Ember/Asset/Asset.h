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
	};

	static AssetType GetAssetTypeFromString(const std::string& type)
	{
		if (type == "Texture")
			return AssetType::Texture;
		if (type == "Mesh")
			return AssetType::Mesh;
		if (type == "Model")
			return AssetType::Model;
		if (type == "Skeleton")
			return AssetType::Skeleton;
		if (type == "Animation")
			return AssetType::Animation;
		if (type == "Shader")
			return AssetType::Shader;
		if (type == "Material")
			return AssetType::Material;
		if (type == "Script")
			return AssetType::Script;

		EB_CORE_ASSERT(false, "Unknown asset type: {}", type);
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