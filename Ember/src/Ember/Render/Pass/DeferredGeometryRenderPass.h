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
		// (Re)caches the G-buffer color/depth attachment IDs into m_TextureOutputs. Must be called
		// after every (re)build of m_GBuffer — a viewport resize deletes the attachment textures and
		// creates new ones with new GL IDs, so cached IDs from Init() go stale and downstream passes
		// would sample the wrong (or a reused) texture.
		void CacheGBufferTextureOutputs();

	private:
		SharedPtr<Framebuffer> m_GBuffer;

		SharedPtr<Texture> m_DefaultWhite, m_DefaultBlack, m_DefaultNormal;
	};

}