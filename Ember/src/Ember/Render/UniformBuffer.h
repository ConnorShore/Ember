#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class UniformBuffer : public SharedResource
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void SetData(const void* data, unsigned int size, unsigned int offset = 0) = 0;

		static SharedPtr<UniformBuffer> Create(unsigned int size, unsigned int bindingPoint);
	};

}