#include "ebpch.h"
#include "CubeMap.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/CubeMap.h"

namespace Ember {

	SharedPtr<CubeMap> CubeMap::Create(uint32_t resolution)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::CubeMap>::Create(resolution);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}