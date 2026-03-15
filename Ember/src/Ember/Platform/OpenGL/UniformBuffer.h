#pragma once

#include "Ember/Render/UniformBuffer.h"

namespace Ember {
	namespace OpenGL {

		class UniformBuffer : public Ember::UniformBuffer
		{
		public:
			UniformBuffer(unsigned int size, unsigned int bindingPoint);
			virtual ~UniformBuffer();

			virtual void SetData(const void* data, unsigned int size, unsigned int offset = 0) override;

		private:
			unsigned int m_Id;
		};

	}
}