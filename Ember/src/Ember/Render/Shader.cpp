#include "ebpch.h"
#include "Shader.h"
#include "RendererAPI.h"

#include "Ember/Core/Core.h"
#include "Ember/Platform/OpenGL/Shader.h"

namespace Ember {

	SharedPtr<Shader> Shader::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:
			EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return SharedPtr<OpenGL::Shader>::Create(filePath);
		}
	}

	SharedPtr<Shader> Shader::Create(const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:
			EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported");
			return nullptr;
		case RendererAPI::API::OpenGL:
			return SharedPtr<OpenGL::Shader>::Create(name, filePath);
		}
	}

	SharedPtr<Shader> ShaderLibrary::Register(const std::string& filePath)
	{
		auto shader = Shader::Create(filePath);
		Add(shader);
		return shader;
	}

	Ember::SharedPtr<Ember::Shader> ShaderLibrary::Register(const std::string& name, std::string& filePath)
	{
		auto shader = Shader::Create(name, filePath);
		Add(shader);
		return shader;
	}

	SharedPtr<Shader> ShaderLibrary::Get(const std::string& name)
	{
		EB_CORE_ASSERT(Exists(name), "Shader does not exists in library!");
		return m_ShaderMap.at(name);
	}

	bool ShaderLibrary::Exists(const std::string& name)
	{
		return m_ShaderMap.find(name) != m_ShaderMap.end();
	}

	void ShaderLibrary::Add(SharedPtr<Shader> shader)
	{
		Add(shader->GetName(), shader);
	}

	void ShaderLibrary::Add(const std::string& name, SharedPtr<Shader> shader)
	{
		EB_CORE_ASSERT(!Exists(name), "Shader already exists in library!");
		m_ShaderMap[name] = shader;
	}

}