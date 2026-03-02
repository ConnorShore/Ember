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

	private:
		static ScopedPtr<RendererAPI> m_RendererApi;
	};
}