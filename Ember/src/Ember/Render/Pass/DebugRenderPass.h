#pragma once

#include "RenderPass.h"

namespace Ember {

	class DebugRenderPass : public RenderPass
	{
	public:
		uint32_t MaxDebugVertices = 20000;

	public:
		DebugRenderPass() = default;
		virtual ~DebugRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Shader> m_DebugShader;
		SharedPtr<VertexArray> m_DebugLineVAO;
		SharedPtr<VertexBuffer> m_DebugLineVBO;
	};

}