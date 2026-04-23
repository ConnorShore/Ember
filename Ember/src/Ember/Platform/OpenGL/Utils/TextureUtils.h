#pragma once

#include "Ember/Render/Texture.h"

#include <glad/glad.h>

namespace Ember::OpenGL {

	inline static uint32_t TextureFormatToOpenGLInternalFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8:			return GL_R8;
		case TextureFormat::RG8:		return GL_RG8;
		case TextureFormat::RGB8:		return GL_RGB8;
		case TextureFormat::RG16F:		return GL_RG16F;
		case TextureFormat::RGB16F:		return GL_RGB16F;
		case TextureFormat::RGBA8:		return GL_RGBA8;
		case TextureFormat::RGBA16F:	return GL_RGBA16F;
		case TextureFormat::RedInteger:	return GL_R32I;
		case TextureFormat::None:		return 0;
		default:
			EB_CORE_ASSERT(false, "Unknown TextureFormat!");
			return 0;
		}
	}

	inline static TextureFormat TextureFormatFromOpenGLInternalFormat(uint32_t internalFormat)
	{
		switch (internalFormat)
		{
		case GL_R8:			return TextureFormat::R8;
		case GL_RG8:		return TextureFormat::RG8;
		case GL_RGB8:		return TextureFormat::RGB8;
		case GL_RG16F:		return TextureFormat::RG16F;
		case GL_RGB16F:		return TextureFormat::RGB16F;
		case GL_RGBA8:		return TextureFormat::RGBA8;
		case GL_RGBA16F:	return TextureFormat::RGBA16F;
		case GL_R32I:		return TextureFormat::RedInteger;
		default:
			EB_CORE_ASSERT(false, "Unknown OpenGL internal format!");
			return TextureFormat::None;
		}
	}

	inline static uint32_t TextureFormatToBytesPerPixel(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8:			return 1;
		case TextureFormat::RG8:		return 2;
		case TextureFormat::RGB8:		return 3;
		case TextureFormat::RG16F:		return 4;
		case TextureFormat::RGB16F:		return 6;
		case TextureFormat::RGBA8:		return 4;
		case TextureFormat::RGBA16F:	return 8;
		case TextureFormat::RedInteger:	return 4;
		case TextureFormat::None:		return 0;
		default:
			EB_CORE_ASSERT(false, "Unknown TextureFormat!");
			return 0;
		}
	}

	inline static GLint BytesPerPixelToGlType(int bytesPerPixel)
	{
		switch (bytesPerPixel)
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4:
		default:
			return GL_RGBA;
		}
	}
}