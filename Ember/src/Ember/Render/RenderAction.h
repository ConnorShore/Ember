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