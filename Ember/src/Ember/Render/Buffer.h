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
		uint32_t Count;
		bool Instanced;
		bool Normalize;
		uint32_t Offset;

		BufferElement(ShaderDataType dataType, const std::string& name, bool instanced = false, bool normalize = false)
			: Name(name), DataType(dataType), Count(ShaderDataTypeCount(dataType)), Instanced(instanced), Normalize(normalize), Offset(0) { }

		BufferElement(ShaderDataType dataType, uint32_t count, bool instanced = false, bool normalize = false)
			: DataType(dataType), Count(count), Instanced(instanced), Normalize(normalize), Offset(0) { }
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

		inline uint32_t GetStride() const { return m_Stride; }
		
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

	private:
		void CalculateStrideAndOffsets();

	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Buffer
	//////////////////////////////////////////////////////////////////////////

	class Buffer : public SharedResource
	{
	public:
		virtual ~Buffer() = default;

		virtual void Bind() const = 0;

		virtual uint32_t GetID() const = 0;
		virtual const size_t GetSize() const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Vertex Buffer
	//////////////////////////////////////////////////////////////////////////

	class VertexBuffer : public Buffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual void SetData(const void* data, uint32_t size) = 0;

		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }
		const BufferLayout& GetLayout() const { return m_Layout; }
		void* GetData() const { return m_Data; }

		static SharedPtr<VertexBuffer> Create(const void* data, uint32_t size);
		static SharedPtr<VertexBuffer> Create(uint32_t size);
		static SharedPtr<VertexBuffer> Create(const void* data, uint32_t size, const BufferLayout& layout);

	protected:
		BufferLayout m_Layout;
		void* m_Data = nullptr;
	};
	

	//////////////////////////////////////////////////////////////////////////
	// Index Buffer
	//////////////////////////////////////////////////////////////////////////
	class IndexBuffer : public Buffer
	{
	public:
		virtual ~IndexBuffer() = default;

		static SharedPtr<IndexBuffer> Create(std::span<const uint32_t> data);
		virtual const size_t GetCount() const = 0;

		uint32_t* GetData() const { return m_Data; }

	protected:
		uint32_t* m_Data = nullptr;
	};

}