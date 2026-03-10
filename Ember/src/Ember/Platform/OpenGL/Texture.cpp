#include "ebpch.h"
#include "Texture.h"
#include "stb_image.h"
#include "Ember/Util/FileUtil.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		Texture::Texture()
			:m_Name("Default"), m_FilePath(), m_Width(1), m_Height(1), m_BytesPerPixel(4), m_LocalBuffer(nullptr)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTextureStorage2D(m_Id, 1, GL_RGBA8, m_Width, m_Height);
		}

		Texture::Texture(const std::string& filePath)
			: Texture(FileUtil::ExtractFileName(filePath), filePath)
		{
		}

		Texture::Texture(const std::string& name, const std::string& filePath)
			: m_Name(name), m_FilePath(filePath), m_Width(0), m_Height(0), m_BytesPerPixel(0)
		{
			stbi_set_flip_vertically_on_load(1);	// Flips texture vertically since OpenGL expects pixels to start on bottom left
			m_LocalBuffer = stbi_load(filePath.c_str(), &m_Width, &m_Height, &m_BytesPerPixel, 4);

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTextureStorage2D(m_Id, 1, GL_RGBA8, m_Width, m_Height);
			glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
		}

		Texture::~Texture()
		{
			glDeleteTextures(1, &m_Id);
			delete m_LocalBuffer;
		}

		void Texture::Bind(unsigned int slot) const
		{
			glBindTextureUnit(slot, m_Id);
		}

		void Texture::SetData(const void* data, unsigned int size)
		{
			glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}

	}
}