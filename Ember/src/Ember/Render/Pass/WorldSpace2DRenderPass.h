#pragma once

#include "RenderPass.h"

namespace Ember {

	class WorldSpace2DRenderPass : public RenderPass
	{
	public:
		void Init() override;
		void Execute(RenderContext& context) override;
		void Shutdown() override;

	private:
		void DrawSprites(AssetManager& assetManager, Registry& registry);
		void DrawText(AssetManager& assetManager, Registry& registry);
	};

}