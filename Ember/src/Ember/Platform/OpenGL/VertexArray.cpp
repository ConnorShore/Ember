#include "ebpch.h"
#include "VertexArray.h"

#include <glad/glad.h>

static GLuint ShaderDataTypeToGLType(Ember::ShaderDataType type)
{
	switch (type)
	{
	case Ember::ShaderDataType::Bool:		return GL_BOOL;
	case Ember::ShaderDataType::Int:		return GL_INT;
	case Ember::ShaderDataType::Int2:		return GL_INT;
	case Ember::ShaderDataType::Int3:		return GL_INT;
	case Ember::ShaderDataType::Int4:		return GL_INT;
	case Ember::ShaderDataType::UInt:		return GL_UNSIGNED_INT;
	case Ember::ShaderDataType::UInt2:		return GL_UNSIGNED_INT;
	case Ember::ShaderDataType::UInt3:		return GL_UNSIGNED_INT;
	case Ember::ShaderDataType::UInt4:		return GL_UNSIGNED_INT;
	case Ember::ShaderDataType::Float:		return GL_FLOAT;
	case Ember::ShaderDataType::Float2:		return GL_FLOAT;
	case Ember::ShaderDataType::Float3:		return GL_FLOAT;
	case Ember::ShaderDataType::Float4:		return GL_FLOAT;
	}

	EB_CORE_ASSERT(false, "Unsupported shader data type for GL type conversion!");
	return 0;
}

static bool IsIntegerType(Ember::ShaderDataType type)
{
	switch (type)
	{
	case Ember::ShaderDataType::Int:
	case Ember::ShaderDataType::Int2:
	case Ember::ShaderDataType::Int3:
	case Ember::ShaderDataType::Int4:
	case Ember::ShaderDataType::UInt:
	case Ember::ShaderDataType::UInt2:
	case Ember::ShaderDataType::UInt3:
	case Ember::ShaderDataType::UInt4:
		return true;
	default:
		return false;
	}
}

namespace Ember {
	namespace OpenGL {

		VertexArray::VertexArray()
			: m_Id(0), m_CurrentVertexBufferInd(0)
		{
			glCreateVertexArrays(1, &m_Id);
		}

		VertexArray::~VertexArray()
		{
			for (uint32_t i = 0; i < m_CurrentVertexBufferInd; i++)
				glVertexArrayAttribBinding(m_Id, i, 0);

			glDeleteVertexArrays(1, &m_Id);
		}

		void VertexArray::Bind() const
		{
			glBindVertexArray(m_Id);
		}

		void VertexArray::AddVertexBuffer(const SharedPtr<VertexBuffer>& vertexBuffer)
		{
			EB_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size() > 0,
				"Vertex buffer must have a layout set before being used in a vertex array!");

			uint32_t vboIndex = m_VertexBuffers.size();

			glVertexArrayVertexBuffer(m_Id, vboIndex, vertexBuffer->GetID(), 0, vertexBuffer->GetLayout().GetStride());
			SetVertexBufferAttribs(vertexBuffer, vboIndex);

			m_VertexBuffers.push_back(vertexBuffer);
		}

		void VertexArray::SetIndexBuffer(const SharedPtr<IndexBuffer>& indexBuffer)
		{
			m_IndexBuffer = indexBuffer;
			glVertexArrayElementBuffer(m_Id, indexBuffer->GetID());
		}

		// Configures vertex attributes using DSA (Direct State Access) based on the buffer layout
		void VertexArray::SetVertexBufferAttribs(const SharedPtr<VertexBuffer>& buffer, uint32_t vboIndex)
		{
			for (auto& elem : buffer->GetLayout())
			{
				glEnableVertexArrayAttrib(m_Id, m_CurrentVertexBufferInd);

				if (IsIntegerType(elem.DataType))
				{
					glVertexArrayAttribIFormat(
						m_Id,
						m_CurrentVertexBufferInd,
						ShaderDataTypeCount(elem.DataType),
						ShaderDataTypeToGLType(elem.DataType),
						elem.Offset
					);
				}
				else
				{
					glVertexArrayAttribFormat(
						m_Id,
						m_CurrentVertexBufferInd,
						ShaderDataTypeCount(elem.DataType),
						ShaderDataTypeToGLType(elem.DataType),
						elem.Normalize,
						elem.Offset
					);
				}

				if (elem.Instanced)
					glVertexArrayBindingDivisor(m_Id, vboIndex, 1);
				
				glVertexArrayAttribBinding(m_Id, m_CurrentVertexBufferInd++, vboIndex);
			}
		}

	}
}