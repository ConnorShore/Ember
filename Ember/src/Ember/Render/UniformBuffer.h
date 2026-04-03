#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class UniformBuffer : public SharedResource
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static SharedPtr<UniformBuffer> Create(uint32_t size, uint32_t bindingPoint);
	};

}