#pragma once

#include "Ember/Core/Core.h"

#include <string>
#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Texture
	//////////////////////////////////////////////////////////////////////////

	class Texture : public SharedResource
	{
	public:
		virtual ~Texture() = default;

		virtual void Bind(unsigned int slot = 0) const = 0;
		virtual void Unbind() const = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual const std::string& GetName() const = 0;
		virtual void SetData(const void* data, unsigned int size) = 0;

		virtual unsigned int GetID() const = 0;

		static SharedPtr<Texture> Create();
		static SharedPtr<Texture> Create(const std::string& filePath);
		static SharedPtr<Texture> Create(const std::string& name, const std::string& filePath);

		virtual bool operator==(const SharedPtr<Texture>& other) const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Texture Library
	//////////////////////////////////////////////////////////////////////////

	class TextureLibrary
	{
	public:
		SharedPtr<Texture> Register(const std::string& filePath);
		SharedPtr<Texture> Register(const std::string& name, const std::string& filePath);

		SharedPtr<Texture> Get(const std::string& name);
		bool Exists(const std::string& name);

	private:
		void Add(SharedPtr<Texture> texture);
		void Add(const std::string& name, SharedPtr<Texture> texture);

	private:
		std::unordered_map<std::string, SharedPtr<Texture>> m_TextureMap;
	};
}