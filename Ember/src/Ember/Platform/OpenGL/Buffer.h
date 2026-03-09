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
			VertexBuffer(const void* data, unsigned int size);
			VertexBuffer(unsigned int size);
			VertexBuffer(const void* data, unsigned int size, const BufferLayout& layout);
			virtual ~VertexBuffer();

			void Bind() const override;

			virtual void SetData(const void* data, unsigned int size) override;

			inline virtual const unsigned int GetID() const override { return m_Id; }
			inline virtual const size_t GetSize() const override { return m_Size; }

		private:
			unsigned int m_Id;
			size_t m_Size;
		};

		//////////////////////////////////////////////////////////////////////////
		// Index Buffer
		//////////////////////////////////////////////////////////////////////////

		class IndexBuffer : public Ember::IndexBuffer
		{
		public:
			IndexBuffer(std::span<const unsigned int> data);
			virtual ~IndexBuffer();

			void Bind() const override;

			inline virtual const unsigned int GetID() const override { return m_Id; }
			inline virtual const size_t GetSize() const override { return m_Size; }
			inline virtual const size_t GetCount() const override { return m_Count; }

		private:
			unsigned int m_Id;
			size_t m_Size;
			size_t m_Count;
		};

	}
}