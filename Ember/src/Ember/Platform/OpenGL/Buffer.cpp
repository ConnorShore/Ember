#include "ebpch.h"
#include "Buffer.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		//////////////////////////////////////////////////////////////////////////
		// Vertex Buffer
		//////////////////////////////////////////////////////////////////////////

		template <Ember::VertexDataType T>
		VertexBuffer<T>::VertexBuffer(std::span<const T> data)
			: m_Id(0), m_Size(data.size_bytes())
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
		}

		template <Ember::VertexDataType T>
		VertexBuffer<T>::VertexBuffer(unsigned int size)
			: m_Id(0), m_Size(size)
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		template <Ember::VertexDataType T>
		VertexBuffer<T>::VertexBuffer(std::span<const T> data, const BufferLayout& layout)
			: m_Id(0), m_Size(data.size_bytes())
		{
			this->m_Layout = layout;
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
		}

		template <Ember::VertexDataType T>
		VertexBuffer<T>::~VertexBuffer()
		{
			glDeleteBuffers(1, &m_Id);
		}

		template <Ember::VertexDataType T>
		void VertexBuffer<T>::Bind() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		}

		template <Ember::VertexDataType T>
		void VertexBuffer<T>::SetData(std::span<const T> data)
		{
			glNamedBufferSubData(m_Id, 0, data.size_bytes(), data.data());
		}

		// Known vertex buffer types
		template class VertexBuffer<int>;
		template class VertexBuffer<float>;
		template class VertexBuffer<double>;

		//////////////////////////////////////////////////////////////////////////
		// Index Buffer
		//////////////////////////////////////////////////////////////////////////

		IndexBuffer::IndexBuffer(std::span<const unsigned int> data)
			: m_Id(0), m_Size(data.size_bytes()), m_Count(data.size())
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
		}

		IndexBuffer::~IndexBuffer()
		{
			glDeleteBuffers(1, &m_Id);
		}

		void IndexBuffer::Bind() const
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
		}
	}
}