#include "ebpch.h"
#include "Shader.h"
#include "RendererAPI.h"

#include "Ember/Core/Core.h"
#include "Ember/Platform/OpenGL/Shader.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Shader
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<Shader> Shader::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Shader> Shader::Create(const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported");  return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Shader>::Create(name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Shader Library
	//////////////////////////////////////////////////////////////////////////

	const SharedPtr<Shader>& ShaderLibrary::Register(const std::string& filePath)
	{
		auto shader = Shader::Create(filePath);
		Add(std::move(shader));
		return Get(std::filesystem::path(filePath).stem().string());
	}

	const SharedPtr<Shader>& ShaderLibrary::Register(const std::string& name, const std::string& filePath)
	{
		auto shader = Shader::Create(name, filePath);
		Add(std::move(shader));
		return Get(name);
	}

	const SharedPtr<Shader>& ShaderLibrary::Get(const std::string& name)
	{
		EB_CORE_ASSERT(Exists(name), "Shader does not exists in library!");
		return m_ShaderMap.at(name);
	}

	bool ShaderLibrary::Exists(const std::string& name)
	{
		return m_ShaderMap.contains(name);
	}

	void ShaderLibrary::Add(SharedPtr<Shader>&& shader)
	{
		Add(shader->GetName(), std::move(shader));
	}

	void ShaderLibrary::Add(const std::string& name, SharedPtr<Shader>&& shader)
	{
		EB_CORE_ASSERT(!Exists(name), "Shader already exists in library!");
		m_ShaderMap[name] = std::move(shader);
	}

}