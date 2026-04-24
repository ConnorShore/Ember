#pragma once

#include "RenderPass.h"

namespace Ember {

	class FinalBlitRenderPass : public RenderPass
	{
	public:
		FinalBlitRenderPass() = default;
		virtual ~FinalBlitRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Shader> m_BlitShader;
		SharedPtr<VertexArray> m_ScreenQuadVAO;
	};

}