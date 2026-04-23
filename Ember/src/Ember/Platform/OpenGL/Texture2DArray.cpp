#include "ebpch.h"
#include "Texture2DArray.h"
#include "Ember/Platform/OpenGL/Utils/TextureUtils.h"

#include "stb_image.h"
#include <glad/glad.h>

namespace Ember::OpenGL {

	Texture2DArray::Texture2DArray(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, uint32_t layers, const void* data, bool clampToEdge)
		: Ember::Texture2DArray(uuid, name, ""), m_Width(width), m_Height(height), m_NumLayers(layers), m_BytesPerPixel(TextureFormatToBytesPerPixel(format)), m_Format(format)
	{
		m_NumMipMaps = 1; // Usually 1 for shadow maps/data arrays, but you can calculate mips if needed

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_Id);
		
		if (clampToEdge)
		{
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			// TODO: May need to pass in wrap mode so can have clamp to edge/border or repeat
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTextureParameterfv(m_Id, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureStorage3D(m_Id, m_NumMipMaps, TextureFormatToOpenGLInternalFormat(format), m_Width, m_Height, m_NumLayers);

		if (data)
		{
			glTextureSubImage3D(m_Id, 0, 0, 0, 0, m_Width, m_Height, m_NumLayers, BytesPerPixelToGlType(m_BytesPerPixel), GL_UNSIGNED_BYTE, data);
		}
	}

	Texture2DArray::~Texture2DArray()
	{
		glDeleteTextures(1, &m_Id);
	}

	void Texture2DArray::Bind(uint32_t slot /*= 0*/) const
	{
		glBindTextureUnit(slot, m_Id);
	}

	void Texture2DArray::GenerateMipmaps() const
	{
		glGenerateTextureMipmap(m_Id);
	}

	void Texture2DArray::SetData(const void* data, uint32_t layerIndex, uint32_t size)
	{
		EB_CORE_ASSERT(layerIndex < m_NumLayers, "Layer index out of bounds!");
		glTextureSubImage3D(m_Id, 0, 0, 0, layerIndex, m_Width, m_Height, 1, BytesPerPixelToGlType(m_BytesPerPixel), GL_UNSIGNED_BYTE, data);
	}

}