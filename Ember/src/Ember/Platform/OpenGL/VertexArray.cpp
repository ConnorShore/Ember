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
	case Ember::ShaderDataType::Float:		return GL_FLOAT;
	case Ember::ShaderDataType::Float2:		return GL_FLOAT;
	case Ember::ShaderDataType::Float3:		return GL_FLOAT;
	case Ember::ShaderDataType::Float4:		return GL_FLOAT;
	}

	EB_CORE_ASSERT(false, "Unsupported shader data type for GL type conversion!");
	return 0;
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

		void VertexArray::SetBuffer(const SharedPtr<VertexBuffer>& vertexBuffer, const SharedPtr<IndexBuffer>& indexBuffer)
		{
			EB_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size() > 0,
				"Vertex buffer must have a layout set before being used in a vertex array!");

			m_VertexBuffer = vertexBuffer;
			m_IndexBuffer = indexBuffer;

			glVertexArrayVertexBuffer(m_Id, 0, vertexBuffer->GetID(), 0, vertexBuffer->GetLayout().GetStride());
			glVertexArrayElementBuffer(m_Id, indexBuffer->GetID());

			SetVertexBufferAttribs();
		}

		void VertexArray::SetVertexBufferAttribs()
		{
			for (auto& elem : m_VertexBuffer->GetLayout())
			{
				glEnableVertexArrayAttrib(m_Id, m_CurrentVertexBufferInd);
				glVertexArrayAttribFormat(
					m_Id, 
					m_CurrentVertexBufferInd, 
					ShaderDataTypeCount(elem.DataType), 
					ShaderDataTypeToGLType(elem.DataType),
					elem.Normalize,
					elem.Offset
				);
				m_CurrentVertexBufferInd++;
			}

			for (uint32_t i = 0; i < m_CurrentVertexBufferInd; i++)
				glVertexArrayAttribBinding(m_Id, i, 0);
		}

	}
}