#include "ebpch.h"
#include "GraphicsContext.h"
#include "RendererAPI.h"
#include "Ember/Platform/OpenGL/GraphicsContext.h"
#include "Ember/Core/Core.h"

namespace Ember {

	ScopedPtr<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (RendererAPI::GetApi()) {
		case RendererAPI::API::OpenGL:
			return ScopedPtr<OpenGL::GraphicsContext>::Create((GLFWwindow*)window);
		case RendererAPI::API::None:
		default:
			EB_CORE_ASSERT(false, "No renderer API has been set");
			return nullptr;
		}
	}

}