#include "ebpch.h"
#include "Framebuffer.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Framebuffer.h"

namespace Ember {

	SharedPtr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& specification)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Framebuffer>::Create(specification);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}