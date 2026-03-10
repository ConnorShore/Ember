#include "ebpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/VertexArray.h"

namespace Ember {

	SharedPtr<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API type is currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::VertexArray>::Create();
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}
}