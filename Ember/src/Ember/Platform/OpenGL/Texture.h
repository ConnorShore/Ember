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
			Texture(const std::string& name, unsigned int width, unsigned int height, const void* data);

			virtual ~Texture();

			void Bind(unsigned int slot = 0) const override;

			virtual void SetData(const void* data, unsigned int size) override;

			inline virtual unsigned int GetWidth() const override { return m_Width; }
			inline virtual unsigned int GetHeight() const override { return m_Height; }
			inline virtual unsigned int GetID() const override { return m_Id; }

			virtual bool operator==(const SharedPtr<Ember::Texture>& other) const override { return m_Id == other->GetID(); }

		private:
			unsigned int m_Id;
			int m_Width, m_Height, m_BytesPerPixel;
			unsigned char* m_LocalBuffer;
		};

	}
}