#include "ebpch.h"
#include "UniformBuffer.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/UniformBuffer.h"

namespace Ember {

	SharedPtr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t bindingPoint)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::UniformBuffer>::Create(size, bindingPoint);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}