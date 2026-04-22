#pragma once

#include "Buffer.h"
#include "Ember/Core/Core.h"

#include <vector>

namespace Ember {

	class VertexArray : public SharedResource
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void AddVertexBuffer(const SharedPtr<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const SharedPtr<IndexBuffer>& indexBuffer) = 0;

		virtual const SharedPtr<VertexBuffer>& GetVertexBuffer(uint32_t index = 0) const = 0;
		virtual const SharedPtr<IndexBuffer>& GetIndexBuffer() const = 0;

		static SharedPtr<VertexArray> Create();
	};

}