#include "ebpch.h"
#include "ColorGradePass.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void ColorGradePass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_ColorGradeShaderEditor = assetManager.GetAsset<Shader>(Constants::Assets::ColorGradeEditorShadUUID);
		m_ColorGradeShaderRuntime = assetManager.GetAsset<Shader>(Constants::Assets::ColorGradeRuntimeShadUUID);
		m_BaseBakedLUTTexture = assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultNeutralColorLUTUUID);
	}

	void ColorGradePass::Render(PostProcessPassContext& context)
	{
		if (context.RenderCtx.IsRuntime)
			RenderRuntime(context.InputBuffer, context.OutputBuffer);
		else
			RenderEditor(context.InputBuffer, context.OutputBuffer);
	}

	void ColorGradePass::RenderEditor(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		outputBuffer->Bind();

		m_ColorGradeShaderEditor->Bind();

		// White balance uniforms
		m_ColorGradeShaderEditor->SetFloat("u_Temperature", Settings.Temperature);
		m_ColorGradeShaderEditor->SetFloat("u_Tint", Settings.Tint);

		// Color adjustment uniforms
		m_ColorGradeShaderEditor->SetFloat("u_Contrast", Settings.Contrast);
		m_ColorGradeShaderEditor->SetFloat("u_Saturation", Settings.Saturation);

		// Lift, Gamma, Gain uniforms
		m_ColorGradeShaderEditor->SetFloat4("u_Lift", Settings.Lift);
		m_ColorGradeShaderEditor->SetFloat4("u_Gamma", Settings.Gamma);
		m_ColorGradeShaderEditor->SetFloat4("u_Gain", Settings.Gain);

		m_ColorGradeShaderEditor->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		m_ColorGradeShaderEditor->SetInt("u_BaseBakedLUT", 1);
		RenderAction::SetTextureUnit(1, m_BaseBakedLUTTexture->GetID());

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void ColorGradePass::RenderRuntime(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		outputBuffer->Bind();

		m_ColorGradeShaderRuntime->Bind();

		m_ColorGradeShaderRuntime->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		m_ColorGradeShaderRuntime->SetInt("u_BakedLUT", 1);
		RenderAction::SetTextureUnit(1, m_ColorGradeLUTBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

}