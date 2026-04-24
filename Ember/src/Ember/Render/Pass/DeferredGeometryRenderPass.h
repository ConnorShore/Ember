#pragma once

#include "RenderPass.h"

namespace Ember {

	class Framebuffer;

	class DeferredGeometryRenderPass : public RenderPass
	{
	public:
		DeferredGeometryRenderPass() = default;
		virtual ~DeferredGeometryRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Framebuffer> m_GBuffer;

		SharedPtr<Texture> m_DefaultWhite, m_DefaultBlack, m_DefaultNormal;
	};

}