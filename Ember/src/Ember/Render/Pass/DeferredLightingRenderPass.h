#pragma once

#include "Ember/Render/Pass/RenderPass.h"

namespace Ember {

	class Framebuffer;
	class Registry;

	class DeferredLightingRenderPass : public RenderPass
	{
	public:
		DeferredLightingRenderPass() = default;
		virtual ~DeferredLightingRenderPass() = default;

		void Init() override;
		void Execute(RenderContext& context) override;
		void Shutdown() override;

	private:
		void RenderDirectionalLights(RenderContext& context, LightDataBlock& lightData, Registry& registry);
		void RenderPointLights(RenderContext& context, LightDataBlock& lightData, Registry& registry);
		void RenderSpotLights(RenderContext& context, LightDataBlock& lightData, Registry& registry);

	private:
		SharedPtr<Framebuffer> m_HdrSceneBuffer;

		SharedPtr<Shader> m_LightingShader;
		SharedPtr<Mesh> m_ScreenQuad;
	};

}