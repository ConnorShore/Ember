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

	inline static int TextureFormatBytesPerPixel(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8: return 1;
		case TextureFormat::RG8: return 2;
		case TextureFormat::RGB8: return 3;
		case TextureFormat::RG16F: return 4;
		case TextureFormat::RGB16F: return 6;
		case TextureFormat::RGBA8: return 4;
		case TextureFormat::RGBA16F: return 8;
		case TextureFormat::RedInteger: return 4;
		case TextureFormat::None: return 0;
		}

		EB_CORE_ASSERT(false, "Unknown TextureFormat!");
	}

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