#pragma once

#include "RenderPass.h"

namespace Ember {

	class ForwardEntitiesRenderPass : public RenderPass
	{
	public:
		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;
	};
}