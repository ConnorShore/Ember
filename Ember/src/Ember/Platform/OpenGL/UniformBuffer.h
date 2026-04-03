#pragma once

#include "Ember/Render/UniformBuffer.h"

namespace Ember {
	namespace OpenGL {

		class UniformBuffer : public Ember::UniformBuffer
		{
		public:
			UniformBuffer(uint32_t size, uint32_t bindingPoint);
			virtual ~UniformBuffer();

			virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		private:
			uint32_t m_Id;
		};

	}
}