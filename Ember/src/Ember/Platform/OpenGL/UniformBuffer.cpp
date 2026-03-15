#include "ebpch.h"
#include "UniformBuffer.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		UniformBuffer::UniformBuffer(unsigned int size, unsigned int bindingPoint)
		{
			glCreateBuffers(1, &m_Id);
			glNamedBufferStorage(m_Id, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
			glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_Id);
		}

		UniformBuffer::~UniformBuffer()
		{
			glDeleteBuffers(1, &m_Id);
		}

		void UniformBuffer::SetData(const void* data, unsigned int size, unsigned int offset /*= 0*/)
		{
			glNamedBufferSubData(m_Id, offset, size, data);
		}

	}
}