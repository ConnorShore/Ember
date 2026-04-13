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
			m_LocalData.resize(size);
			std::memcpy(m_LocalData.data(), data, size);

			this->m_Data = m_LocalData.data();

			// Immutable storage with DYNAMIC_STORAGE allows glNamedBufferSubData updates
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, data, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::VertexBuffer(uint32_t size)
			: m_Id(0), m_Size(size)
		{
			m_LocalData.resize(size, 0);
			this->m_Data = m_LocalData.data();

			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::VertexBuffer(const void* data, uint32_t size, const BufferLayout& layout)
			: m_Id(0), m_Size(size)
		{
			this->m_Layout = layout;

			m_LocalData.resize(size);
			std::memcpy(m_LocalData.data(), data, size);
			this->m_Data = m_LocalData.data();

			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, data, GL_DYNAMIC_STORAGE_BIT);
		}

		VertexBuffer::~VertexBuffer()
		{
			this->m_Data = nullptr;
			glDeleteBuffers(1, &m_Id);
		}

		void VertexBuffer::Bind() const
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_Id);
		}

		void VertexBuffer::SetData(const void* data, uint32_t size)
		{
			this->m_Data = const_cast<void*>(data);
			glNamedBufferSubData(m_Id, 0, size, data);
		}

		//////////////////////////////////////////////////////////////////////////
		// Index Buffer
		//////////////////////////////////////////////////////////////////////////

		IndexBuffer::IndexBuffer(std::span<const uint32_t> data)
			: m_Id(0), m_Size(data.size_bytes()), m_Count(data.size())
		{
			m_LocalData.assign(data.begin(), data.end());
			this->m_Data = m_LocalData.data();

			// Immutable storage with DYNAMIC_STORAGE allows glNamedBufferSubData updates
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, data.size_bytes(), data.data(), GL_DYNAMIC_STORAGE_BIT);
		}

		IndexBuffer::~IndexBuffer()
		{
			this->m_Data = nullptr;
			glDeleteBuffers(1, &m_Id);
		}

		void IndexBuffer::Bind() const
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Id);
		}
	}
}