#include "ebpch.h"
#include "CubeMap.h"
#include "stb_image.h"

#include "Ember/Render/Texture.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		CubeMap::CubeMap(uint32_t resolution)
			: Ember::CubeMap(UUID(), "EmptyCubeMap", ""), m_Width(resolution), m_Height(resolution)
		{
			// Generate the number of mipmap levels based on the resolution of the cubemap\
			// (e.g., 1024 -> 11 levels. 128 -> 8 levels)
			m_NumMipMaps = 1 + (int)std::floor(std::log2(resolution));

			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_Id);

			glTextureStorage2D(m_Id, m_NumMipMaps, GL_RGBA16F, m_Width, m_Height);

			glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_Id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}

		CubeMap::~CubeMap()
		{
			glDeleteTextures(1, &m_Id);
		}

		void CubeMap::Bind(uint32_t slot /*= 0*/) const
		{
			glBindTextureUnit(slot, m_Id);
		}

		void CubeMap::GenerateMipmaps() const
		{
			glGenerateTextureMipmap(m_Id);
		}

		void CubeMap::SetData(CubeMapFace face, const void* data, uint32_t size)
		{
			int faceIndex = (int)face - 1;
			glTextureSubImage3D(m_Id, 0, 0, 0, faceIndex, m_Width, m_Height, 1, GL_RGBA, GL_FLOAT, data);
		}

	}
}