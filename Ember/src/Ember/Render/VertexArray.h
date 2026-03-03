#pragma once

namespace Ember {

	class VertexArray {
	public:
		VertexArray(/*vertexBuffer, indexBuffer*/);
		virtual ~VertexArray();

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

	private:
		unsigned int m_Id;
	};

}