#include "ebpch.h"
#include "Texture.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Texture.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture 2D
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<Texture> Texture::Create()
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create();
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create(filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<Texture> Texture::Create(const std::string& name, const std::string& filePath)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Renderer API specified. This is currently unsupported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::Texture>::Create(name, filePath);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Texture Library
	//////////////////////////////////////////////////////////////////////////

	const SharedPtr<Texture>& TextureLibrary::Register(const std::string& filePath)
	{
		auto texture = Texture::Create(filePath);
		Add(std::move(texture));
		return Get(std::filesystem::path(filePath).stem().string());
	}

	const SharedPtr<Texture>& TextureLibrary::Register(const std::string& name, const std::string& filePath)
	{
		auto texture = Texture::Create(name, filePath);
		Add(std::move(texture));
		return Get(name);
	}

	const SharedPtr<Texture>& TextureLibrary::Get(const std::string& name)
	{
		EB_CORE_ASSERT(Exists(name), "Texture does not exists in library!");
		return m_TextureMap.at(name);
	}

	bool TextureLibrary::Exists(const std::string& name)
	{
		return m_TextureMap.contains(name);
	}

	void TextureLibrary::Add(SharedPtr<Texture>&& texture)
	{
		Add(texture->GetName(), std::move(texture));
	}

	void TextureLibrary::Add(const std::string& name, SharedPtr<Texture>&& texture)
	{
		EB_CORE_ASSERT(!Exists(name), "Texture already exists in library!");
		m_TextureMap[name] = std::move(texture);
	}

}