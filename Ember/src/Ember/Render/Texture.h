#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Asset/Asset.h"

#include <string>
#include <unordered_map>

namespace Ember {

	// TODO: integrate texture type into textures
	enum class TextureType
	{
		None = 0,
		Diffuse,
		Specular,
		Normal,
	};

	//////////////////////////////////////////////////////////////////////////
	// Texture
	//////////////////////////////////////////////////////////////////////////

	class Texture : public Asset
	{
	public:
		Texture(const std::string& name, const std::string& filePath)
			: Asset(name, filePath, AssetType::Texture) { }

		virtual ~Texture() = default;

		virtual void Bind(unsigned int slot = 0) const = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetData(const void* data, unsigned int size) = 0;

		virtual unsigned int GetID() const = 0;

		static SharedPtr<Texture> Create();
		static SharedPtr<Texture> Create(const std::string& filePath);
		static SharedPtr<Texture> Create(const std::string& name, const std::string& filePath);
		static SharedPtr<Texture> Create(const std::string& name, unsigned int width, unsigned int height, const void* data);

		virtual bool operator==(const SharedPtr<Texture>& other) const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Texture Library
	//////////////////////////////////////////////////////////////////////////

	class TextureImporter
	{
	public:
		static SharedPtr<Texture> Load(const std::string& filePath)
		{
			return Texture::Create(filePath);
		}
		static SharedPtr<Texture> Load(const std::string& name, const std::string& filePath)
		{
			return Texture::Create(name, filePath);
		}
	};
}