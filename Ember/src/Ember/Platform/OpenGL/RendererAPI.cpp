#include "ebpch.h"
#include "RendererAPI.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		void RendererAPI::Clear()
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void RendererAPI::SetClearColor(Vector4<float> color)
		{
			glClearColor(color[0], color[1], color[2], color[3]);
		}

		void RendererAPI::UseFaceCulling(bool use)
		{
			use ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		}

		void RendererAPI::UseDepthTest(bool use)
		{
			use ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		}

		void RendererAPI::UseBlending(bool use)
		{
			if (use)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glDisable(GL_BLEND);
			}
		}

		void RendererAPI::SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int  height)
		{
			glViewport(x, y, width, height);
		}

		void RendererAPI::DrawIndexed(const SharedPtr<VertexArray>& vertexArray)
		{
			DrawIndexed(vertexArray, vertexArray->GetIndexBuffer()->GetCount());
		}

		void RendererAPI::DrawIndexed(const SharedPtr<VertexArray>& vertexArray, unsigned int indicesCt)
		{
			vertexArray->Bind();
			glDrawElements(GL_TRIANGLES, indicesCt, GL_UNSIGNED_INT, nullptr);
		}
	}
}