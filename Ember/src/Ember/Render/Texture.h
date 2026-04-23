#pragma once

#include "Ember/Asset/Asset.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture Format Utils
	//////////////////////////////////////////////////////////////////////////

	enum class TextureFormat
	{
		None = 0,
		R8,
		RG8,
		RGB8,
		RG16F,
		RGB16F,
		RGBA8,
		RGBA16F,
		RedInteger
	};

	// TODO: integrate texture type into textures
	enum class TextureType
	{
		None = 0,
		Diffuse,
		Specular,
		Normal,
		CubeMap
	};

	class Texture : public Asset
	{
	public:
		Texture(const std::string& name, const std::string& filePath)
			: Asset(name, filePath, GetStaticType()) {
		}
		Texture(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, GetStaticType()) { }

		virtual ~Texture() = default;

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual void GenerateMipmaps() const = 0;
		virtual uint32_t GetNumMipMapLevels() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetID() const = 0;

		static AssetType GetStaticType() { return AssetType::Texture; }
	};

}

// TODO: Move texture library here