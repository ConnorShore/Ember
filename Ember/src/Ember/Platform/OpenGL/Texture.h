#pragma once

#include "Ember/Render/Texture.h"

#include <string.h>

namespace Ember {
	namespace OpenGL {

		class Texture : public Ember::Texture
		{
		public:
			Texture();
			Texture(const std::string& filePath);
			Texture(const std::string& name, const std::string& filePath);
			Texture(UUID uuid, const std::string& name, const std::string& filePath);
			Texture(const std::string& name, uint32_t width, uint32_t height, const void* data);
			Texture(UUID uuid, const std::string& name, uint32_t width, uint32_t height, const void* data);

			virtual ~Texture();

			void Bind(uint32_t slot = 0) const override;

			virtual void SetData(const void* data, uint32_t size) override;

			inline virtual uint32_t GetWidth() const override { return m_Width; }
			inline virtual uint32_t GetHeight() const override { return m_Height; }
			inline virtual uint32_t GetID() const override { return m_Id; }

			virtual bool operator==(const SharedPtr<Ember::Texture>& other) const override { return m_Id == other->GetID(); }

		private:
			uint32_t m_Id;
			int m_Width, m_Height, m_BytesPerPixel;
			unsigned char* m_LocalBuffer;
		};

	}
}