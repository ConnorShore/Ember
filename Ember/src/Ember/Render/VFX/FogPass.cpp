#include "ebpch.h"
#include "FogPass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void FogPass::Init()
	{
		Enabled = false;	// Disabled by default
		m_FogShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::FogShadUUID);
	}

	void FogPass::Render(PostProcessPassContext& context)
	{
		context.OutputBuffer->Bind();

		// Clear the output buffer
		RenderAction::Clear();

		m_FogShader->Bind();

		// Pass the uniforms from your Environment Settings
		m_FogShader->SetFloat("u_FogDensity", Settings.Density);
		m_FogShader->SetFloat3("u_FogColor", Settings.Color);
		m_FogShader->SetFloat("u_FogStart", Settings.StartDistance);

		// Pass the active camera's clipping planes
		float cameraNearClip = context.RenderCtx.ActiveCamera->GetNearClip();
		float cameraFarClip = context.RenderCtx.ActiveCamera->GetFarClip();
		m_FogShader->SetFloat("u_NearClip", cameraNearClip);
		m_FogShader->SetFloat("u_FarClip", cameraFarClip);

		// Bind the Ping-Pong Scene Color
		m_FogShader->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, context.InputBuffer->GetColorAttachmentID(0));

		// Bind scene depth
		m_FogShader->SetInt("u_Depth", 1);
		RenderAction::SetTextureUnit(1, context.DepthBuffer->GetDepthAttachmentID());

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

}