#include "ebpch.h"
#include "Shader.h"
#include "RendererAPI.h"

#include "Ember/Core/Core.h"
#include "Ember/Platform/OpenGL/Shader.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Shader
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<Shader> Shader::Create(const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(filePath, macros);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Shader> Shader::Create(const std::string& name, const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported");  return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(name, filePath, macros);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Shader> Shader::Create(UUID uuid, const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(uuid, filePath, macros);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Shader> Shader::Create(UUID uuid, const std::string& name, const std::string& filePath, const ShaderMacros& macros /* = {} */)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported");  return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(uuid, name, filePath, macros);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}