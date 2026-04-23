#include "ebpch.h"
#include "Texture2DArray.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Texture2DArray.h"

namespace Ember {

	SharedPtr<Texture2DArray> Texture2DArray::Create(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, uint32_t layers, const void* data, bool clampToEdge /* = false */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:    EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:  return SharedPtr<OpenGL::Texture2DArray>::Create(uuid, name, format, width, height, layers, data, clampToEdge);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}