#pragma once

#include "RenderPass.h"

namespace Ember {

	class SkyboxRenderPass : public RenderPass
	{
	public:
		SkyboxRenderPass() = default;
		virtual ~SkyboxRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Shader> m_SkyboxShader;
		SharedPtr<VertexArray> m_CubeVAO;
	};

}