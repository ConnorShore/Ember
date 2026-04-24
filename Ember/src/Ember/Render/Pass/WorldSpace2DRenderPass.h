#pragma once

#include "RenderPass.h"

namespace Ember {

	class WorldSpace2DRenderPass : public RenderPass
	{
	public:
		WorldSpace2DRenderPass() = default;
		virtual ~WorldSpace2DRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		void DrawSprites(AssetManager& assetManager, Registry& registry);
		void DrawText(AssetManager& assetManager, Registry& registry);
	};

}