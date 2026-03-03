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

	void ShaderLibrary::Add(SharedPtr<Shader> shader)
	{
		EB_CORE_ASSERT(!Exists(shader), "Shader already exists in library!");
		m_ShaderMap[shader->GetName()] = shader;
	}

	SharedPtr<Shader> ShaderLibrary::Get(const std::string& name)
	{
		return m_ShaderMap.at(name);
	}

	bool ShaderLibrary::Exists(SharedPtr<Shader> shader)
	{
		return m_ShaderMap.find(shader->GetName()) != m_ShaderMap.end();
	}

	SharedPtr<Shader> ShaderLibrary::Load(const std::string& filePath)
	{
		auto shader = Shader::Create(filePath);
		Add(shader);
		return shader;
	}

}