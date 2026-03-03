#pragma once

#include "Ember/Core/Core.h"

namespace Ember {

	class VertexBuffer
	{
	public:
		VertexBuffer(const void* data, unsigned int size);
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;

		static SharedPtr<VertexBuffer> Create(const void* data, unsigned int size);
	};

	class IndexBuffer
	{
	public:
		IndexBuffer(const unsigned int* data, unsigned int count);
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;

		static SharedPtr<IndexBuffer> Create(const unsigned int* data, unsigned int count);
	};

	class IndexedVertexBuffer
	{
	public:
		IndexedVertexBuffer();
		virtual ~IndexedVertexBuffer() = default;

		virtual void Bind() const = 0;
	};

}