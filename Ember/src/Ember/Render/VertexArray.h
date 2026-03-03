#pragma once

#include "Buffer.h"
#include "Ember/Core/Core.h"

#include <vector>

namespace Ember {

	class VertexArray {
	public:
		VertexArray(/*vertexBuffer, indexBuffer*/);
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void AddVertexBuffer(const SharedPtr<VertexBuffer>& vertexBuffer) = 0;
		virtual void AddIndexBuffer(const SharedPtr<IndexBuffer>& indexBuffer) = 0;

	private:
		unsigned int m_Id, m_CurrentVertexBufferInd = 0;
		std::vector<SharedPtr<VertexBuffer>> m_VertexBuffers;
		SharedPtr<IndexBuffer> m_IndexBuffer;
	};

}