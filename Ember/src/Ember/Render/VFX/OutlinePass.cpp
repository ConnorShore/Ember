#include "ebpch.h"
#include "OutlinePass.h"
#include "Ember/Core/Application.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void OutlinePass::Init()
	{
		m_OutlineShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::OutlineShad);
	}

	void OutlinePass::Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		//if (m_SelectedEntityID == Constants::Entities::InvalidEntityID || m_GBuffer == nullptr || m_HdrBuffer == nullptr)
		//	return;

		outputBuffer->Bind();
		RenderAction::SetClearColor(Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(RendererAPI::RenderBit::Color);

		m_OutlineShader->Bind();

		// Feed the shader the scene color, bright channel, and entity ID buffers from
		// both the GBuffer (opaque) and HDR buffer (forward) so outlines work on all objects
		m_OutlineShader->SetInt("u_Scene", 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		m_OutlineShader->SetInt("u_BrightScene", 1);
		RenderAction::SetTextureUnit(1, inputBuffer->GetColorAttachmentID(1));

		// Bind Opaque IDs (G-Buffer)
		m_OutlineShader->SetInt("u_OpaqueIDBuffer", 2);
		RenderAction::SetTextureUnit(2, m_GBuffer->GetColorAttachmentID(4));	// RedInteger attachment

		// Bind Forward IDs (HDR Buffer)
		m_OutlineShader->SetInt("u_ForwardIDBuffer", 3);
		RenderAction::SetTextureUnit(3, m_HdrBuffer->GetColorAttachmentID(2));  // Forward RedInteger attachment

		// Handle depth so outlines don't render over closer objects
		m_OutlineShader->SetInt("u_DepthBuffer", 4);
		RenderAction::SetTextureUnit(4, m_HdrBuffer->GetDepthAttachmentID());

		m_OutlineShader->SetInt("u_SelectedEntityID", m_SelectedEntityID);
		m_OutlineShader->SetFloat3("u_OutlineColor", m_OutlineColor);
		m_OutlineShader->SetFloat("u_OutlineThickness", m_OutlineThickness);

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());

		outputBuffer->Unbind();
	}

}