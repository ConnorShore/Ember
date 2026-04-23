#include "ebpch.h"
#include "Texture2D.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Texture2D.h"

#include <glad/glad.h>

namespace Ember {

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