#include "ebpch.h"
#include "Texture.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Texture.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture 2D
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<Texture> Texture::Create()
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create();
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(const std::string& name, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture>::Create(name, width, height, data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture>::Create(uuid, name, width, height, data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create(filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create(name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(UUID uuid, const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create(uuid, name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}
}