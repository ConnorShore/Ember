#pragma once

#include "RenderPass.h"

namespace Ember {

	class EditorGridRenderPass : public RenderPass
	{
	public:
		EditorGridRenderPass() = default;
		virtual ~EditorGridRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Shader> m_GridShader;
		SharedPtr<Mesh> m_ScreenQuad;
	};

}