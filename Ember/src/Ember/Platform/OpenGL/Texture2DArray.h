#pragma once

#include "Ember/Render/Texture2DArray.h"

#include <string.h>

namespace Ember {
	namespace OpenGL {

		class Texture2DArray : public Ember::Texture2DArray
		{
		public:
			Texture2DArray(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, uint32_t layers, const void* data, bool clampToEdge = false);

			virtual ~Texture2DArray();

			virtual void Bind(uint32_t slot = 0) const override;
			virtual void GenerateMipmaps() const override;

			virtual void SetData(const void* data, uint32_t index, uint32_t size) override;

			inline virtual TextureFormat GetFormat() const override { return m_Format; }
			inline virtual uint32_t GetNumMipMapLevels() const override { return m_NumMipMaps; }

			inline virtual uint32_t GetWidth() const override { return m_Width; }
			inline virtual uint32_t GetHeight() const override { return m_Height; }
			inline virtual uint32_t GetID() const override { return m_Id; }

			virtual bool operator==(const SharedPtr<Ember::Texture2DArray>& other) const override { return m_Id == other->GetID(); }

		private:
			uint32_t m_Id, m_NumLayers, m_NumMipMaps;
			int m_Width, m_Height, m_BytesPerPixel;
			TextureFormat m_Format;
		};

	}
}