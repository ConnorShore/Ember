#include "ebpch.h"
#include "FXAAPass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Core/Application.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void FXAAPass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_FXAAShader = assetManager.GetAsset<Shader>(Constants::Assets::FXAAShad);
	}

	void FXAAPass::Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		outputBuffer->Bind();

		m_FXAAShader->Bind();

		m_FXAAShader->SetFloat2(Constants::Uniforms::InvViewportDimensions, m_InvViewportDimensions);
		m_FXAAShader->SetFloat(Constants::Uniforms::EdgeThresholdMin, EdgeThresholdMin);
		m_FXAAShader->SetFloat(Constants::Uniforms::EdgeThresholdMax, EdgeThresholdMax);
		m_FXAAShader->SetFloat(Constants::Uniforms::SubpixelQuality, SubpixelQuality);

		m_FXAAShader->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}
}