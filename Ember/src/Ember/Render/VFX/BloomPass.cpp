#include "ebpch.h"
#include "BloomPass.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Core/Application.h"

namespace Ember {

	void BloomPass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_BloomPrefilterShader = assetManager.GetAsset<Shader>(Constants::Assets::BloomPrefilterShad);
		m_BlurShader = assetManager.GetAsset<Shader>(Constants::Assets::GaussianBlurShad);
		m_BloomCompositeShader = assetManager.GetAsset<Shader>(Constants::Assets::BloomShad);

		// Internal buffer to extract the bright areas of the scene
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F
			};
			m_BloomExtractionBuffer = Framebuffer::Create(specs);
		}
		// Internal ping pong buffer for the blur
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F
			};
			for (uint32_t i = 0; i < m_PingPongBuffers.size(); i++)
				m_PingPongBuffers[i] = Framebuffer::Create(specs);
		}
	}

	void BloomPass::Render(SharedPtr<Framebuffer> inputBuffer, SharedPtr<Framebuffer> outputBuffer)
	{
		// Bloom Prefilter Pass //
		m_BloomExtractionBuffer->Bind();

		RenderAction::SetViewport(0, 0, m_BloomExtractionBuffer->GetSpecification().Width, m_BloomExtractionBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		m_BloomPrefilterShader->Bind();
		m_BloomPrefilterShader->SetFloat(Constants::Uniforms::Threshold, Threshold);
		m_BloomPrefilterShader->SetFloat(Constants::Uniforms::Knee, Knee);

		m_BloomPrefilterShader->SetInt(Constants::Uniforms::Scene, 0);
		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());

		m_BloomExtractionBuffer->Unbind();
		////

		// BLUR PASS //
		RenderAction::UseDepthTest(false);

		bool horizontalPass = true, firstIter = true;

		m_BlurShader->Bind();
		m_BlurShader->SetInt(Constants::Uniforms::Image, 0);
		for (uint32_t i = 0; i < m_Passes; i++)
		{
			m_PingPongBuffers[horizontalPass]->Bind();

			RenderAction::SetViewport(0, 0, m_PingPongBuffers[horizontalPass]->GetSpecification().Width, m_PingPongBuffers[horizontalPass]->GetSpecification().Height);

			m_BlurShader->SetFloat(Constants::Uniforms::BlurRadius, BlurRadius);
			m_BlurShader->SetInt(Constants::Uniforms::HorizontalPass, horizontalPass);
			if (firstIter)
				RenderAction::SetTextureUnit(0, m_BloomExtractionBuffer->GetColorAttachmentID(0));
			else
				RenderAction::SetTextureUnit(0, m_PingPongBuffers[!horizontalPass]->GetColorAttachmentID(0));

			Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
			horizontalPass = !horizontalPass;
			if (firstIter)
				firstIter = false;
		}
		//////

		// Bloom Pass //
		outputBuffer->Bind();

		// Apply the bloom effect by blending the blurred bright areas back onto the scene
		RenderAction::SetViewport(0, 0, outputBuffer->GetSpecification().Width, outputBuffer->GetSpecification().Height);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(false);

		m_BloomCompositeShader->Bind();

		m_BloomCompositeShader->SetFloat(Constants::Uniforms::BloomIntensity, Intensity);

		m_BloomCompositeShader->SetInt(Constants::Uniforms::Scene, 0);
		m_BloomCompositeShader->SetInt(Constants::Uniforms::BloomBlur, 1);

		RenderAction::SetTextureUnit(0, inputBuffer->GetColorAttachmentID(0));
		RenderAction::SetTextureUnit(1, m_PingPongBuffers[!horizontalPass]->GetColorAttachmentID(0));

		Renderer3D::Submit(m_ScreenQuad->GetVertexArray());
		/////
	}

	void BloomPass::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_BloomExtractionBuffer->ViewportResize(width, height);
		m_PingPongBuffers[0]->ViewportResize(width, height);
		m_PingPongBuffers[1]->ViewportResize(width, height);
	}

}