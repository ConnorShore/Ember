#include "ebpch.h"
#include "VignettePass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void VignettePass::Init()
	{
		Enabled = false;	// Disabled by default
		m_VignetteShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::VignetteShadUUID);
	}

	void VignettePass::Render(PostProcessPassContext& context)
	{
		context.OutputBuffer->Bind();

		m_VignetteShader->Bind();

		m_VignetteShader->SetFloat("u_AspectRatio", context.RenderCtx.ActiveCamera->GetAspectRatio());
		m_VignetteShader->SetFloat("u_Intensity", Settings.Intensity);
		m_VignetteShader->SetFloat("u_Size", Settings.Size);
		m_VignetteShader->SetFloat("u_Smoothness", Settings.Smoothness);
		m_VignetteShader->SetFloat3("u_Color", Settings.Color);

		m_VignetteShader->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, context.InputBuffer->GetColorAttachmentID(0));
		
		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
		context.OutputBuffer->Unbind();
	}

}