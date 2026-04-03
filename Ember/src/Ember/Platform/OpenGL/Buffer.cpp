#include "ebpch.h"
#include "Buffer.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		//////////////////////////////////////////////////////////////////////////
		// Vertex Buffer
		//////////////////////////////////////////////////////////////////////////

		VertexBuffer::VertexBuffer(const void* data, uint32_t size)
			: m_Id(0), m_Size(size)
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, data, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::VertexBuffer(uint32_t size)
			: m_Id(0), m_Size(size)
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::VertexBuffer(const void* data, uint32_t size, const BufferLayout& layout)
			: m_Id(0), m_Size(size)
		{
			this->m_Layout = layout;
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, data, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::~VertexBuffer()
		{
			glDeleteBuffers(1, &m_Id);
		}

		void VertexBuffer::Bind() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		}

		void VertexBuffer::SetData(const void* data, uint32_t size)
		{
			glNamedBufferSubData(m_Id, 0, size, data);
		}

		//////////////////////////////////////////////////////////////////////////
		// Index Buffer
		//////////////////////////////////////////////////////////////////////////

		IndexBuffer::IndexBuffer(std::span<const uint32_t> data)
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