#pragma once

#include "Ember/Render/VertexArray.h"

#include <vector>

namespace Ember {
	namespace OpenGL {

		class VertexArray : public Ember::VertexArray
		{
		public:
			VertexArray();
			virtual ~VertexArray();

			virtual void Bind() const override;
			virtual void AddVertexBuffer(const SharedPtr<VertexBuffer>& vertexBuffer) override;
			virtual void SetIndexBuffer(const SharedPtr<IndexBuffer>& indexBuffer) override;

			inline virtual const SharedPtr<VertexBuffer>& GetVertexBuffer(uint32_t index = 0) const override { return m_VertexBuffers[index]; }
			inline virtual const SharedPtr<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

		private:
			void SetVertexBufferAttribs(const SharedPtr<VertexBuffer>& buffer, uint32_t vboIndex);

		private:
			uint32_t m_Id, m_CurrentVertexBufferInd;

			std::vector<SharedPtr<VertexBuffer>> m_VertexBuffers;
			SharedPtr<IndexBuffer> m_IndexBuffer = nullptr;
		};

	}
}