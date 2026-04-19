#pragma once

#include "Texture.h"

#include "Ember/Core/Core.h"
#include "Ember/Asset/Asset.h"

#include <string>
#include <unordered_map>

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

	uint32_t TextureFormatToOpenGLInternalFormat(TextureFormat format);
	TextureFormat TextureFormatFromOpenGLInternalFormat(uint32_t internalFormat);

	uint32_t TextureFormatToBytesPerPixel(TextureFormat format);

	//////////////////////////////////////////////////////////////////////////
	// Texture 2D
	//////////////////////////////////////////////////////////////////////////

	class Texture2D : public Texture
	{
	public:
		Texture2D(const std::string& name, const std::string& filePath)
			: Texture(name, filePath) {
		}
		Texture2D(UUID uuid, const std::string& name, const std::string& filePath)
			: Texture(uuid, name, filePath) {
		}

		virtual ~Texture2D() = default;

		virtual void SetData(const void* data, uint32_t size) = 0;
		virtual TextureFormat GetFormat() const = 0;

		static SharedPtr<Texture2D> Create();
		static SharedPtr<Texture2D> Create(const std::string& filePath);
		static SharedPtr<Texture2D> Create(const std::string& name, const std::string& filePath);
		static SharedPtr<Texture2D> Create(UUID uuid, const std::string& name, const std::string& filePath);
		static SharedPtr<Texture2D> Create(const std::string& name, uint32_t width, uint32_t height, const void* data);
		static SharedPtr<Texture2D> Create(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, const void* data, bool clampToEdge = false);
		static SharedPtr<Texture2D> Create(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data);

		virtual bool operator==(const SharedPtr<Texture2D>& other) const = 0;
	};
}