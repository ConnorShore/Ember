#pragma once

#include "RenderPass.h"

#include "Ember/Math/Math.h"

#include <array>

namespace Ember {

	class Framebuffer;

	class ShadowRenderPass : public RenderPass
	{
	public:
		ShadowRenderPass() = default;
		virtual ~ShadowRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		void CreateDirectionalShadowMap(RenderContext& context);
		void CreateSpotlightShadowMap(RenderContext& context);
		void RenderGeometryForShadowMaps(RenderContext& context, const Matrix4f& lightViewMatrix, const SharedPtr<Framebuffer>& shadowMapBuffer);

	private:
		SharedPtr<Framebuffer> m_DirectionalShadowMapBuffer;
		SharedPtr<Framebuffer> m_SpotShadowMapBuffer;

		SharedPtr<Shader> m_ShadowShader;
		SharedPtr<Shader> m_SkinnedShadowShader;

		Matrix4f m_DirectionalLightSpaceMatrices[3];
		Matrix4f m_SpotLightViewMatrix;

		// TODO: Probably expose some of these variables to be able to be modified via UI
		uint32_t m_CascadeCount = 3;
		uint32_t m_ShadowMapResolution = 4096;
		float m_BlendOverlap = 3.0f; // TODO: This must match frag shader, move to uniform

		// These are the actual distance values from the camera. 
		// Cascade 0: CameraNear -> 15.0f
		// Cascade 1: 15.0f -> 60.0f
		// Cascade 2: 60.0f -> 300.0f or CameraFar
		std::array<float, 3> m_ShadowCascadeLevels = { 5.0f, 40.0f, 300.0f };
	};

}