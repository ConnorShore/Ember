#pragma once

#include "RenderPass.h"

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
		SharedPtr<Framebuffer> RenderHDRPasses(RenderContext& context, SharedPtr<Framebuffer> currentHdrInput, SharedPtr<Framebuffer> currentHdrOutput);
		void RenderToneMapping(RenderContext& context, SharedPtr<Framebuffer>& currentHdrInput);
		SharedPtr<Framebuffer>& RenderLDRPasses(RenderContext& context, SharedPtr<Framebuffer>& currentLdrInput, SharedPtr<Framebuffer>& currentLdrOutput);
		void BlitToScreen(RenderContext& context, SharedPtr<Framebuffer>& currentLdrInput);

	private:
		SharedPtr<Framebuffer> m_PostProcessBufferA, m_PostProcessBufferB;
		SharedPtr<Framebuffer> m_LdrBufferA, m_LdrBufferB;
		SharedPtr<Shader> m_BlitShader;
		SharedPtr<VertexArray> m_ScreenQuadVAO;

		PostProcessStack& m_PostProcessStack;
	};

}