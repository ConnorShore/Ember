#include "ebpch.h"
#include "RendererAPI.h"

#include <glad/glad.h>

namespace Ember {
	namespace OpenGL {

		static GLuint RenderBitsToGLBits(RendererAPI::RenderBits bits)
		{
			GLuint result = 0;
			if (bits & static_cast<RendererAPI::RenderBits>(RendererAPI::RenderBit::Color)) result |= GL_COLOR_BUFFER_BIT;
			if (bits & static_cast<RendererAPI::RenderBits>(RendererAPI::RenderBit::Depth)) result |= GL_DEPTH_BUFFER_BIT;
			return result;
		}

		void RendererAPI::Clear()
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void RendererAPI::Clear(RenderBits bits)
		{
			glClear(RenderBitsToGLBits(bits));
		}

		void RendererAPI::SetClearColor(Vector4<float> color)
		{
			glClearColor(color[0], color[1], color[2], color[3]);
		}

		void RendererAPI::UseFaceCulling(bool use)
		{
			use ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		}

		void RendererAPI::CullFace(Face face)
		{
			switch (face)
			{
			case RendererAPI::Face::Back:	glCullFace(GL_BACK); break;
			case RendererAPI::Face::Front:	glCullFace(GL_FRONT); break;
			default: EB_CORE_ASSERT(false, "Unknown face specified!");
			}
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

		void RendererAPI::UseDepthMask(bool use)
		{
			if (use)
			{
				glDepthMask(GL_TRUE);
			}
			else
			{
				glDepthMask(GL_FALSE);
			}
		}

		void RendererAPI::SetTextureUnit(unsigned int unit, unsigned int texture)
		{
			glBindTextureUnit(unit, texture);
		}

		void RendererAPI::SetFramebuffer(unsigned int framebufferId)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
		}

		void RendererAPI::GetPreviousFramebuffer(int* outFramebufferId)
		{
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, outFramebufferId);
		}

		void RendererAPI::CopyDepthBuffer(unsigned int gBufferId, unsigned int outputBuffer, Vector4<int> viewportDims)
		{
			// get viewport dims in x0, y0, x1, y1 (not x,y,width,height)
			int x0 = viewportDims.x;
			int y0 = viewportDims.y;
			int x1 = viewportDims.x + viewportDims.z;
			int y1 = viewportDims.y + viewportDims.w;

			glBlitNamedFramebuffer(gBufferId, outputBuffer,
				x0, y0, x1, y1,
				x0, y0, x1, y1,
				GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}

		void RendererAPI::GetViewportDimensions(int* outViewportDims)
		{
			glGetIntegerv(GL_VIEWPORT, outViewportDims);
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