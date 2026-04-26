#pragma once

#include "Ember/Core/Core.h"
#include "Texture2D.h"

#include "stb_image_write.h"

namespace Ember {

	class TextureImporter
	{
	public:
		inline static bool Save(const SharedPtr<Texture2D>& texture, const std::string& filePath)
		{
			stbi_flip_vertically_on_write(true);
			return stbi_write_png(filePath.c_str(), texture->GetWidth(), texture->GetHeight(), 
				TextureFormatBytesPerPixel(texture->GetFormat()), texture->GetData(), 256 * 3);
		}

		inline static SharedPtr<Texture2D> Load(const std::string& filePath)
		{
			return Texture2D::Create(filePath);
		}
		inline static SharedPtr<Texture2D> Load(const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(name, filePath);
		}
		inline static SharedPtr<Texture2D> Load(UUID uuid, const std::string& name, const std::string& filePath)
		{
			return Texture2D::Create(uuid, name, filePath);
		}
	};

}