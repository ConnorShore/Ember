#include "ebpch.h"
#include "ColorGradePass.h"

#include "Ember/Core/Application.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void ColorGradePass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_ColorGradeShaderEditor = assetManager.GetAsset<Shader>(Constants::Assets::ColorGradeEditorShadUUID);
		m_ColorGradeShaderRuntime = assetManager.GetAsset<Shader>(Constants::Assets::ColorGradeRuntimeShadUUID);
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

		m_ColorGradeShaderEditor->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

	void ColorGradePass::RenderRuntime(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		outputBuffer->Bind();

		m_ColorGradeShaderRuntime->Bind();

		// Set uniforms here

		m_ColorGradeShaderRuntime->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
	}

}