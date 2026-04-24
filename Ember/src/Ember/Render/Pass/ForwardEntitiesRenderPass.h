#pragma once

#include "RenderPass.h"

namespace Ember {

	class ForwardEntitiesRenderPass : public RenderPass
	{
	public:
		void Init() override;
		void Execute(RenderContext& context) override;
		void Shutdown() override;

	private:

	};
}