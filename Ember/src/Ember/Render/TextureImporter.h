#pragma once

#include "Ember/Core/Core.h"
#include "Texture2D.h"

namespace Ember {

	class TextureImporter
	{
	public:
		static SharedPtr<Texture2D> Load(const std::string& filePath)
		{
			return Texture2D::Create(filePath);
		}
		static SharedPtr<Texture2D> Load(const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(name, filePath);
		}
		static SharedPtr<Texture2D> Load(UUID uuid, const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(uuid, name, filePath);
		}
	};

}