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

		static inline void UseDepthTest(bool use)
		{
			m_RendererApi->UseDepthTest(use);
		}

		static inline void UseBlending(bool use)
		{
			m_RendererApi->UseBlending(use);
		}

		static inline void SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
		{
			m_RendererApi->SetViewport(x, y, width, height);
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