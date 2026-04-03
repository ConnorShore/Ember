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
			virtual void SetBuffer(const SharedPtr<VertexBuffer>& vertexBuffer, const SharedPtr<IndexBuffer>& indexBuffer) override;

			inline virtual const SharedPtr<VertexBuffer>& GetVertexBuffer() const override { return m_VertexBuffer; }
			inline virtual const SharedPtr<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

		private:
			void SetVertexBufferAttribs();

		private:
			uint32_t m_Id, m_CurrentVertexBufferInd;

			SharedPtr<VertexBuffer> m_VertexBuffer;
			SharedPtr<IndexBuffer> m_IndexBuffer;
		};

	}
}