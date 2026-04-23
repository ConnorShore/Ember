#include "ebpch.h"
#include "Texture2D.h"
#include "Ember/Platform/OpenGL/Utils/TextureUtils.h"

#include "stb_image.h"
#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		Texture2D::Texture2D()
			: Texture2D("Default", 1, 1, nullptr)
		{
		}

		Texture2D::Texture2D(const std::string& filePath)
			: Texture2D(std::filesystem::path(filePath).stem().string(), filePath)
		{
		}

		Texture2D::Texture2D(UUID uuid, const std::string& name, const std::string& filePath)
			: Ember::Texture2D(uuid, name, filePath), m_Width(0), m_Height(0), m_BytesPerPixel(0)
		{
			stbi_set_flip_vertically_on_load(1);
			bool isHDR = stbi_is_hdr(filePath.c_str());

			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			if (isHDR)
			{
				float* data = stbi_loadf(filePath.c_str(), &m_Width, &m_Height, &m_BytesPerPixel, 4);
				if (data)
				{
					m_NumMipMaps = 1 + (int)std::floor(std::log2(std::max(m_Width, m_Height)));
					glTextureStorage2D(m_Id, m_NumMipMaps, GL_RGBA16F, m_Width, m_Height);
					glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, data);
					stbi_image_free(data);
				}
			}
			else
			{
				// Standard LDR load
				unsigned char* data = stbi_load(filePath.c_str(), &m_Width, &m_Height, &m_BytesPerPixel, 4);
				if (data)
				{
					m_NumMipMaps = 1 + (int)std::floor(std::log2(std::max(m_Width, m_Height)));
					glTextureStorage2D(m_Id, m_NumMipMaps, GL_RGBA8, m_Width, m_Height);
					glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, data);
					stbi_image_free(data);
				}
			}
		}

		Texture2D::Texture2D(const std::string& name, const std::string& filePath)
			: Texture2D(UUID(), name, filePath)
		{
		}

		Texture2D::Texture2D(const std::string& name, uint32_t width, uint32_t height, const void* data)
			: Texture2D(UUID(), name, width, height, data)
		{
		}

		Texture2D::Texture2D(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, const void* data, bool clampToEdge /* = false */)
			: Ember::Texture2D(uuid, name, ""), m_Width(width), m_Height(height), m_BytesPerPixel(TextureFormatToBytesPerPixel(format)), m_Format(format)
		{
			m_NumMipMaps = 1 + (int)std::floor(std::log2(std::max(width, height)));
			glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

			if (clampToEdge)
			{
				glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureStorage2D(m_Id, m_NumMipMaps, TextureFormatToOpenGLInternalFormat(format), m_Width, m_Height);

			// Swizzle Trick for Font Atlases
			if (m_BytesPerPixel == 1)
			{
				glTextureParameteri(m_Id, GL_TEXTURE_SWIZZLE_R, GL_ONE);
				glTextureParameteri(m_Id, GL_TEXTURE_SWIZZLE_G, GL_ONE);
				glTextureParameteri(m_Id, GL_TEXTURE_SWIZZLE_B, GL_ONE);
				glTextureParameteri(m_Id, GL_TEXTURE_SWIZZLE_A, GL_RED);
			}

			if (data)
				glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, BytesPerPixelToGlType(m_BytesPerPixel), GL_UNSIGNED_BYTE, data);
		}

		Texture2D::Texture2D(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data)
			: Texture2D(uuid, name, TextureFormat::RGBA8, width, height, data)
		{
		}

		Texture2D::~Texture2D()
		{
			glDeleteTextures(1, &m_Id);
		}

		void Texture2D::Bind(uint32_t slot) const
		{
			glBindTextureUnit(slot, m_Id);
		}

		void Texture2D::GenerateMipmaps() const
		{
			glGenerateTextureMipmap(m_Id);
		}

		void Texture2D::SetData(const void* data, uint32_t size)
		{
			glTextureSubImage2D(m_Id, 0, 0, 0, m_Width, m_Height, BytesPerPixelToGlType(m_BytesPerPixel), GL_UNSIGNED_BYTE, data);
		}

	}
}