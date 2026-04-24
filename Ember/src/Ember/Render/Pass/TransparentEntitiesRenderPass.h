#pragma once

#include "RenderPass.h"

namespace Ember {

	class RenderForwardEntitiesRenderPass : public RenderPass
	{
	public:
		inline virtual void Init() override {}
		inline virtual void Execute(RenderContext& context) override {}
		inline virtual void Shutdown() override {}
	};
}