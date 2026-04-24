#pragma once

#include "RenderPass.h"

namespace Ember {

	class SkyboxRenderPass : public RenderPass
	{
	public:
		SkyboxRenderPass() = default;
		virtual ~SkyboxRenderPass() = default;

		void Init() override;
		void Execute(RenderContext& context) override;
		void Shutdown() override;

	private:
		SharedPtr<Shader> m_SkyboxShader;
		SharedPtr<VertexArray> m_CubeVAO;

	};

}