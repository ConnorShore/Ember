#pragma once

#include "RenderPass.h"

namespace Ember {

	class Framebuffer;

	class DeferredGeometryRenderPass : public RenderPass
	{
	public:
		DeferredGeometryRenderPass() = default;
		virtual ~DeferredGeometryRenderPass() = default;

		void Init() override;
		void Execute(RenderContext& context) override;
		void Shutdown() override;

	private:
		SharedPtr<Framebuffer> m_GBuffer;

		SharedPtr<Texture> m_DefaultWhite, m_DefaultBlack, m_DefaultNormal;
	};

}