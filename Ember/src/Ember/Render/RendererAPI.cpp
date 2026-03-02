#include "ebpch.h"
#include "RendererAPI.h"
#include "Ember/Platform/OpenGL/RendererAPI.h"

namespace Ember {

	RendererAPI::API RendererAPI::s_Api = RendererAPI::API::OpenGL;

	ScopedPtr<RendererAPI> RendererAPI::Create()
	{
		switch (s_Api)
		{
		case Ember::RendererAPI::API::OpenGL:
			return ScopedPtr<OpenGL::RendererAPI>::Create();
		case Ember::RendererAPI::API::None:
		default:
			EB_CORE_ASSERT(false, "Invalid render API selected!");
			return nullptr;
		}
	}
}