#include "ebpch.h"
#include "ToneMapPass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

namespace Ember {

	void ToneMapPass::Init()
	{
		m_ToneMapShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::ToneMapShadUUID);
		m_ScreenQuadVAO = PrimitiveGenerator::CreateQuad(2.0f, 2.0f)->GetVertexArray();
	}

	void ToneMapPass::Render(PostProcessPassContext& context)
	{
		context.OutputBuffer->Bind();
		RenderAction::SetViewport(context.RenderCtx.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		m_ToneMapShader->Bind();
		m_ToneMapShader->SetFloat(Constants::Uniforms::Exposure, Exposure);
		m_ToneMapShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, context.InputBuffer->GetColorAttachmentID(0));
		Renderer3D::Submit(m_ScreenQuadVAO);
		context.OutputBuffer->Unbind();
	}

}