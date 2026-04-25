#pragma once

#include "RenderPass.h"
#include "Ember/Render/VFX/PostProcessPass.h"

namespace Ember {

	class Framebuffer;
	class Shader;
	class PostProcessPass;

	using PostProcessStack = std::vector<SharedPtr<PostProcessPass>>;

	class PostProcessRenderPass : public RenderPass
	{
	public:
		PostProcessRenderPass(PostProcessStack& postProcessStack)
			: m_PostProcessStack(postProcessStack) { }

		virtual ~PostProcessRenderPass() = default;

		virtual void Init() override;
		virtual void Execute(RenderContext& context) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void Shutdown() override;

	private:
		SharedPtr<Framebuffer> RenderHDRPasses(PostProcessPassContext& passContext);
		void RenderToneMapping(PostProcessPassContext& passContext);
		SharedPtr<Framebuffer>& RenderLDRPasses(PostProcessPassContext& passContext);

	private:
		SharedPtr<Framebuffer> m_PostProcessBufferA, m_PostProcessBufferB;
		SharedPtr<Framebuffer> m_LdrBufferA, m_LdrBufferB;
		SharedPtr<Shader> m_BlitShader, m_ToneMapShader;
		SharedPtr<VertexArray> m_ScreenQuadVAO;

		PostProcessStack& m_PostProcessStack;
	};

}