#pragma once

#include "Ember/Core/Core.h"

#include "UUID.h"

#include <string>

namespace Ember {

	enum class AssetType
	{
		Texture,
		Mesh,
		Shader,
		Material
	};

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

		inline const std::string& GetFilePath() const { return m_FilePath; }
		inline void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
	protected:
		UUID m_UUID;
		AssetType m_Type;
		std::string m_Name;
		std::string m_FilePath;
	};

}