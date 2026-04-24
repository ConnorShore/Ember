#pragma once

#include "RenderPass.h"

namespace Ember {

	class TransparentEntitiesRenderPass : public RenderPass
	{
	public:
		inline virtual void Init() override {}
		inline virtual void Execute(RenderContext& context) override {}
		inline virtual void OnViewportResize(uint32_t width, uint32_t height) override {}
		inline virtual void Shutdown() override {}
	};
}