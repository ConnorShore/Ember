#pragma once

#include "Ember/Render/Buffer.h"

#include <span>

namespace Ember {
	namespace OpenGL {

		//////////////////////////////////////////////////////////////////////////
		// Vertex Buffer
		//////////////////////////////////////////////////////////////////////////

		class VertexBuffer : public Ember::VertexBuffer
		{
		public:
			VertexBuffer(const void* data, uint32_t size);
			VertexBuffer(uint32_t size);
			VertexBuffer(const void* data, uint32_t size, const BufferLayout& layout);
			virtual ~VertexBuffer();

			void Bind() const override;

			virtual void SetData(const void* data, uint32_t size) override;

			inline virtual uint32_t GetID() const override { return m_Id; }
			inline virtual const size_t GetSize() const override { return m_Size; }

		private:
			uint32_t m_Id;
			size_t m_Size;
		};

		//////////////////////////////////////////////////////////////////////////
		// Index Buffer
		//////////////////////////////////////////////////////////////////////////

		class IndexBuffer : public Ember::IndexBuffer
		{
		public:
			IndexBuffer(std::span<const uint32_t> data);
			virtual ~IndexBuffer();

			void Bind() const override;

			inline virtual uint32_t GetID() const override { return m_Id; }
			inline virtual const size_t GetSize() const override { return m_Size; }
			inline virtual const size_t GetCount() const override { return m_Count; }

		private:
			uint32_t m_Id;
			size_t m_Size;
			size_t m_Count;
		};

	}
}