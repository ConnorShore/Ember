#pragma once

#include "Ember/Render/Texture2D.h"

#include <string.h>

namespace Ember {
	namespace OpenGL {

		class Texture2D : public Ember::Texture2D
		{
		public:
			Texture2D();
			Texture2D(const std::string& filePath);
			Texture2D(const std::string& name, const std::string& filePath);
			Texture2D(UUID uuid, const std::string& name, const std::string& filePath);
			Texture2D(const std::string& name, uint32_t width, uint32_t height, const void* data);
			Texture2D(UUID uuid, const std::string& name, TextureFormat format, uint32_t width, uint32_t height, const void* data, bool clampToEdge = false);
			Texture2D(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data);

			virtual ~Texture2D();

			virtual void Bind(uint32_t slot = 0) const override;
			virtual void GenerateMipmaps() const override;

			virtual void SetData(const void* data, uint32_t size) override;

			inline virtual TextureFormat GetFormat() const override { return m_Format; }
			inline virtual uint32_t GetNumMipMapLevels() const override { return m_NumMipMaps; }

			inline virtual uint32_t GetWidth() const override { return m_Width; }
			inline virtual uint32_t GetHeight() const override { return m_Height; }
			inline virtual uint32_t GetID() const override { return m_Id; }

			virtual bool operator==(const SharedPtr<Ember::Texture2D>& other) const override { return m_Id == other->GetID(); }

		private:
			uint32_t m_Id, m_NumMipMaps;
			int m_Width, m_Height, m_BytesPerPixel;
			TextureFormat m_Format;
		};

	}
}