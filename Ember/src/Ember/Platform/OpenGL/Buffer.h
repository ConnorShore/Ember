#pragma once

#include "Ember/Render/Buffer.h"

#include <span>

namespace Ember {
	namespace OpenGL {

		//////////////////////////////////////////////////////////////////////////
		// Vertex Buffer
		//////////////////////////////////////////////////////////////////////////

		template <Ember::VertexDataType T>
		class VertexBuffer : public Ember::VertexBuffer<T>
		{
		public:
			VertexBuffer(std::span<const T> data);
			VertexBuffer(unsigned int size);
			VertexBuffer(std::span<const T> data, const BufferLayout& layout);
			virtual ~VertexBuffer();

			void Bind() const override;

			virtual void SetData(std::span<const T> data) override;

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