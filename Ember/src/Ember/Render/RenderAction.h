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

		static inline void UseCubeMapSeamless(bool use)
		{
			m_RendererApi->UseCubeMapSeamless(use);
		}

		static inline void UseDepthMask(bool use)
		{
			m_RendererApi->UseDepthMask(use);
		}

		static inline void UseDepthFunction(RendererAPI::DepthFunction func)
		{
			m_RendererApi->UseDepthFunction(func);
		}

		static inline void UseScissorTest(bool use)
		{
			m_RendererApi->UseScissorTest(use);
		}

		static inline bool IsScissorTestEnabled()
		{
			return m_RendererApi->IsScissorTestEnabled();
		}

		static inline void SetTextureUnit(uint32_t unit, uint32_t texture)
		{
			m_RendererApi->SetTextureUnit(unit, texture);
		}

		static inline void SetFramebuffer(uint32_t framebufferId)
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

		static inline void CopyDepthBuffer(uint32_t gBufferId, uint32_t outputBuffer, Vector4<int> viewportDims)
		{
			m_RendererApi->CopyDepthBuffer(gBufferId, outputBuffer, viewportDims);
		}

		static inline void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			m_RendererApi->SetViewport(x, y, width, height);
		}

		static inline void SetViewport(Vector4<int> dimensions)
		{
			m_RendererApi->SetViewport(dimensions.x, dimensions.y, dimensions.z, dimensions.w);
		}

		static inline void DrawIndexed(const SharedPtr<VertexArray>& vertexArray, uint32_t indicesCt)
		{
			m_RendererApi->DrawIndexed(vertexArray, indicesCt);
		}

		static inline void DrawIndexed(const SharedPtr<VertexArray>& vertexArray)
		{
			m_RendererApi->DrawIndexed(vertexArray);
		}

		static inline void DrawIndexedInstanced(const SharedPtr<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
		{
			m_RendererApi->DrawIndexedInstanced(vertexArray, indexCount, instanceCount);
		}

		static inline void DrawLines(const SharedPtr<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			m_RendererApi->DrawLines(vertexArray, vertexCount);
		}

	private:
		static ScopedPtr<RendererAPI> m_RendererApi;
	};
}