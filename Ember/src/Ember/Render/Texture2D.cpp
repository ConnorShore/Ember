#include "ebpch.h"
#include "Texture2D.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Texture2D.h"

#include <glad/glad.h>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture Format Utils
	//////////////////////////////////////////////////////////////////////////

	uint32_t TextureFormatToOpenGLInternalFormat(TextureFormat format)
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

	TextureFormat TextureFormatFromOpenGLInternalFormat(uint32_t internalFormat)
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

	uint32_t TextureFormatToBytesPerPixel(TextureFormat format)
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

	//////////////////////////////////////////////////////////////////////////
	// Texture 2D
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<Texture2D> Texture2D::Create()
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture2D>::Create();
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(const std::string& name, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture2D>::Create(name, width, height, data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, const void* data, bool clampToEdge /* = false */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture2D>::Create(uuid, name, format, width, height, data, clampToEdge);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture2D>::Create(uuid, name, width, height, data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture2D>::Create(filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture2D>::Create(name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture2D> Texture2D::Create(UUID uuid, const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture2D>::Create(uuid, name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}