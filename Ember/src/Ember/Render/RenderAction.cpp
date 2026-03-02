#include "ebpch.h"
#include "RenderAction.h"

namespace Ember {
	ScopedPtr<RendererAPI> RenderAction::m_RendererApi = RendererAPI::Create();
}