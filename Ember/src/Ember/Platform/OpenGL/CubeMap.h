#pragma once

#include "Ember/Render/CubeMap.h"

namespace Ember {
	namespace OpenGL {

		class CubeMap : public Ember::CubeMap
		{
		public:
			CubeMap(uint32_t resolution);
			virtual ~CubeMap();

			virtual void Bind(uint32_t slot = 0) const override;
			virtual void GenerateMipmaps() const override;

			inline virtual uint32_t GetNumMipMapLevels() const override { return m_NumMipMaps; }

			virtual void SetData(CubeMapFace face, const void* data, uint32_t size) override;

			inline virtual uint32_t GetWidth() const override { return m_Width; }
			inline virtual uint32_t GetHeight() const override { return m_Height; }
			inline virtual uint32_t GetID() const override { return m_Id; }

			inline virtual bool operator==(const SharedPtr<Ember::CubeMap>& other) const override { return m_Id == other->GetID(); }

		private:
			uint32_t m_Id, m_NumMipMaps;
			int m_Width, m_Height;
		};

	}
}