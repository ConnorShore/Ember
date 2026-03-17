#pragma once

#include "RendererAPI.h"
#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class RenderAction
	{
	public:
		static inline void Clear()
		{
			m_RendererApi->Clear();
		}

		static inline void Clear(RendererAPI::RenderBit bit)
		{
			m_RendererApi->Clear(bit);
		}

		static inline void Clear(RendererAPI::RenderBits bits)
		{
			m_RendererApi->Clear(bits);
		}

		static inline void SetClearColor(Vector4<float> color)
		{
			m_RendererApi->SetClearColor(color);
		}

		static inline void UseFaceCulling(bool use)
		{
			m_RendererApi->UseFaceCulling(use);
		}

		static inline void CullFace(RendererAPI::Face face)
		{
			m_RendererApi->CullFace(face);
		}

		static inline void UseDepthTest(bool use)
		{
			m_RendererApi->UseDepthTest(use);
		}

		static inline void UseBlending(bool use)
		{
			m_RendererApi->UseBlending(use);
		}

		static inline void SetTextureUnit(unsigned int unit, unsigned int texture)
		{
			m_RendererApi->SetTextureUnit(unit, texture);
		}

		static inline void SetFramebuffer(unsigned int framebufferId)
		{
			m_RendererApi->SetFramebuffer(framebufferId);
		}

		static inline void GetPreviousFramebuffer(int* outFramebufferId)
		{
			m_RendererApi->GetPreviousFramebuffer(outFramebufferId);
		}

		static inline void GetViewportDimensions(int* outViewportdims)
		{
			m_RendererApi->GetViewportDimensions(outViewportdims);
		}

		static inline void CopyDepthBuffer(unsigned int gBufferId, unsigned int outputBuffer, Vector4<int> viewportDims)
		{
			m_RendererApi->CopyDepthBuffer(gBufferId, outputBuffer, viewportDims);
		}

		static inline void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
		{
			m_RendererApi->SetViewport(x, y, width, height);
		}

		static inline void SetViewport(Vector4<int> dimensions)
		{
			m_RendererApi->SetViewport(dimensions.x, dimensions.y, dimensions.z, dimensions.w);
		}

		static inline void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, unsigned int indicesCt)
		{
			m_RendererApi->DrawIndexed(vertexArray, indicesCt);
		}

		static inline void DrawIndexed(const SharedPtr<VertexArray>& vertexArray)
		{
			m_RendererApi->DrawIndexed(vertexArray);
		}

	private:
		static ScopedPtr<RendererAPI> m_RendererApi;
	};
}