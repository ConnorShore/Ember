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

	template <VertexDataType T>
	SharedPtr<VertexBuffer<T>> VertexBuffer<T>::Create(std::span<const T> data)
	{
		switch (RendererAPI::GetApi())
		{
		case RendererAPI::API::None:	EB_CORE_ASSERT(false, "No Render API type currently not supported"); return nullptr;
		case RendererAPI::API::OpenGL:	return SharedPtr<OpenGL::VertexBuffer<T>>::Create(data);
		}

		EB_CORE_ASSERT(false, "Unknown Renderer API selected!");
		return nullptr;
	}

	// Known vertex buffer types
	template SharedPtr<VertexBuffer<int>> VertexBuffer<int>::Create(std::span<const int> data);
	template SharedPtr<VertexBuffer<float>> VertexBuffer<float>::Create(std::span<const float> data);
	template SharedPtr<VertexBuffer<double>> VertexBuffer<double>::Create(std::span<const double> data);

	//////////////////////////////////////////////////////////////////////////
	// Index Buffer
	//////////////////////////////////////////////////////////////////////////

	SharedPtr<IndexBuffer> IndexBuffer::Create(std::span<const unsigned int> data)
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