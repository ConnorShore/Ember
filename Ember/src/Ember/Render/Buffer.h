#pragma once

namespace Ember {

	class VertexBuffer
	{
	public:
		VertexBuffer(const void* data, unsigned int size);
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;

	private:
		unsigned int m_Id;
	};

	class IndexBuffer
	{
		IndexBuffer(const unsigned int* data, unsigned int size);
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;

	private:
		unsigned int m_Id;
	};

}