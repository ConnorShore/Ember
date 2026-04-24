#include "ebpch.h"
#include "FinalBlitRenderPass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void FinalBlitRenderPass::Init()
	{
		m_BlitShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::BlitShad);
		m_ScreenQuadVAO = PrimitiveGenerator::CreateQuad(2.0f, 2.0f)->GetVertexArray();
	}

	void FinalBlitRenderPass::Execute(RenderContext& context)
	{
		//RenderAction::SetFramebuffer(m_OutputBufferId);
		RenderAction::SetViewport(context.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);

		m_BlitShader->Bind();

		m_BlitShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, m_TextureInputs["FinalScene"]);
		Renderer3D::Submit(m_ScreenQuadVAO);
	}

	void FinalBlitRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void FinalBlitRenderPass::Shutdown()
	{
	}

}