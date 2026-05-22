#pragma once

#include "RenderPass.h"

namespace Ember {
	
	class Scene;
	class Entity;

	class ScreenSpace2DRenderPass : public RenderPass
	{
	public:
		ScreenSpace2DRenderPass() = default;
		virtual ~ScreenSpace2DRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		void RenderSprites(Scene* scene, bool drawAll, EntityID selectedEntity, float viewportWidth, float viewportHeight);
		void RenderText(Scene* scene, bool drawAll, EntityID selectedEntity, float viewportWidth, float viewportHeight);

		Entity FindNearestCanvasAncestor(Scene* scene, Entity entity);
	};

}