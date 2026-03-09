#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Render/ShaderType.h"

#include <concepts>
#include <span>
#include <string>
#include <vector>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Buffer Element
	//////////////////////////////////////////////////////////////////////////

	struct BufferElement
	{
		std::string Name;
		ShaderDataType DataType;
		unsigned int Count;
		bool Normalize;
		unsigned int Offset;

		BufferElement(ShaderDataType dataType, const std::string& name, bool normalize = false)
			: Name(name), DataType(dataType), Count(ShaderDataTypeCount(dataType)), Normalize(normalize), Offset(0) { }

		BufferElement(ShaderDataType dataType, unsigned int count, bool normalize)
			: DataType(dataType), Count(count), Normalize(normalize), Offset(0) { }
	};

	//////////////////////////////////////////////////////////////////////////
	// Buffer Layout
	//////////////////////////////////////////////////////////////////////////

	class BufferLayout
	{
	public:
		BufferLayout() = default;
		BufferLayout(std::initializer_list<BufferElement> elements);
		~BufferLayout();

		inline const unsigned int GetStride() const { return m_Stride; }
		
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

	private:
		void CalculateStrideAndOffsets();

	private:
		std::vector<BufferElement> m_Elements;
		unsigned int m_Stride = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Buffer
	//////////////////////////////////////////////////////////////////////////

	class Buffer : public SharedResource
	{
	public:
		virtual ~Buffer() = default;

		virtual void Bind() const = 0;

		virtual const unsigned int GetID() const = 0;
		virtual const size_t GetSize() const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Vertex Buffer Base
	//////////////////////////////////////////////////////////////////////////
	class VertexBufferBase : public Buffer
	{
	public:
		virtual ~VertexBufferBase() = default;

		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }
		const BufferLayout& GetLayout() const { return m_Layout; }

	protected:
		BufferLayout m_Layout;
	};

	//////////////////////////////////////////////////////////////////////////
	// Vertex Buffer
	//////////////////////////////////////////////////////////////////////////

	// TODO: Make it so VertexBuffer isn't templated

	template <typename T>
	concept VertexDataType = std::same_as<T, int> || std::same_as<T, float> || std::same_as<T, double>;

	template <VertexDataType T>
	class VertexBuffer : public VertexBufferBase
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void SetData(std::span<const T> data) = 0;

		static SharedPtr<VertexBuffer<T>> Create(std::span<const T> data);
		static SharedPtr<VertexBuffer<T>> Create(unsigned int size);
		static SharedPtr<VertexBuffer<T>> Create(std::span<const T> data, const BufferLayout& layout);
	};
	

	//////////////////////////////////////////////////////////////////////////
	// Index Buffer
	//////////////////////////////////////////////////////////////////////////
	class IndexBuffer : public Buffer
	{
	public:
		virtual ~IndexBuffer() = default;

		static SharedPtr<IndexBuffer> Create(std::span<const unsigned int> data);
		virtual const size_t GetCount() const = 0;
	};

	// TODO:
	//////////////////////////////////////////////////////////////////////////
	// Indexed Vertex Buffer
	//////////////////////////////////////////////////////////////////////////
	//class IndexedVertexBuffer : public Buffer
	//{
	//public:
	//	virtual ~IndexedVertexBuffer() = default;

	//	static SharedPtr<IndexedVertexBuffer<T>> Create(const T* vertexData, const unsigned int* indexData);
	//};

}