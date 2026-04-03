#include "ebpch.h"
#include "Buffer.h"
#include "RendererAPI.h"

#include "Ember/Platform/OpenGL/Buffer.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Buffer Layout
	//////////////////////////////////////////////////////////////////////////

	BufferLayout::BufferLayout(std::initializer_list<BufferElement> elements)
		: m_Elements(elements)
	{
		CalculateStrideAndOffsets();
	}

	BufferLayout::~BufferLayout()
	{
	}

	void BufferLayout::CalculateStrideAndOffsets()
	{
		for (auto& element : m_Elements)
		{
			element.Offset = m_Stride;
			m_Stride += ShaderDataTypeSize(element.DataType);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Vertex Buffer
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::VertexBuffer>::Create(data, size);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::VertexBuffer>::Create(size);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	SharedPtr<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size, const BufferLayout& layout)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::VertexBuffer>::Create(data, size, layout);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Index Buffer
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<IndexBuffer> IndexBuffer::Create(std::span<const uint32_t> data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::IndexBuffer>::Create(data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

}