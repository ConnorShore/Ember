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

		void RendererAPI::UseCubeMapSeamless(bool use)
		{
			if (use)
				glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
			else
				glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}

		void RendererAPI::UseDepthFunction(DepthFunction func)
		{
			switch (func)
			{
			case DepthFunction::Less:	glDepthFunc(GL_LESS); break;
			case DepthFunction::LessEqual:	glDepthFunc(GL_LEQUAL); break;
			case DepthFunction::Greater:	glDepthFunc(GL_GREATER); break;
			case DepthFunction::GreaterEqual:	glDepthFunc(GL_GEQUAL); break;
			case DepthFunction::Equal:	glDepthFunc(GL_EQUAL); break;
			case DepthFunction::NotEqual:	glDepthFunc(GL_NOTEQUAL); break;
			default: EB_CORE_ASSERT(false, "Unknown depth function specified!");
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

		void RendererAPI::SetTextureUnit(uint32_t unit, uint32_t texture)
		{
			glBindTextureUnit(unit, texture);
		}

		void RendererAPI::SetFramebuffer(uint32_t framebufferId)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
		}

		void RendererAPI::GetPreviousFramebuffer(int* outFramebufferId)
		{
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, outFramebufferId);
		}

		void RendererAPI::CopyDepthBuffer(uint32_t gBufferId, uint32_t outputBuffer, Vector4<int> viewportDims)
		{
			// Convert (x, y, width, height) to (x0, y0, x1, y1) for glBlitNamedFramebuffer
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

		void RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			glViewport(x, y, width, height);
		}

		void RendererAPI::DrawIndexed(const SharedPtr<VertexArray>& vertexArray)
		{
			DrawIndexed(vertexArray, static_cast<uint32_t>(vertexArray->GetIndexBuffer()->GetCount()));
		}

		void RendererAPI::DrawIndexed(const SharedPtr<VertexArray>& vertexArray, uint32_t indicesCt)
		{
			vertexArray->Bind();
			glDrawElements(GL_TRIANGLES, indicesCt, GL_UNSIGNED_INT, nullptr);
		}
	}
}