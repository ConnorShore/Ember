#include "ebpch.h"
#include "PostProcessRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

#include "Ember/Render/VFX/PostProcessPass.h"
#include "Ember/Render/VFX/OutlinePass.h"

namespace Ember {

	void PostProcessRenderPass::Init()
	{
		// Post Process Framebuffers
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RGBA16F,
				Ember::FramebufferTextureFormat::RedInteger,
				Ember::FramebufferTextureFormat::Depth24Stencil8
			};
			m_PostProcessBufferA = Framebuffer::Create(specs);
			m_PostProcessBufferB = Framebuffer::Create(specs);
		}

		// LDR Post Process Buffers (
		// For FXAA, Vignette, etc.)
		{
			Ember::FramebufferSpecification specs;
			specs.Width = 1;
			specs.Height = 1;
			specs.AttachmentSpecs = {
				Ember::FramebufferTextureFormat::RGBA8, // Standard 0-1 colors!
				Ember::FramebufferTextureFormat::Depth24Stencil8
			};
			m_LdrBufferA = Framebuffer::Create(specs);
			m_LdrBufferB = Framebuffer::Create(specs);
		}

		m_ScreenQuadVAO = PrimitiveGenerator::CreateQuad(2.0f, 2.0f)->GetVertexArray();
		m_BlitShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::BlitShad);
	}

	void PostProcessRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();
		RenderAction::UseDepthTest(false);

		// Render HDR Passes
		auto& currentHdrInput = RenderHDRPasses(context, m_FramebufferInputs["HDRScene"], m_PostProcessBufferA);

		// Tone Mapping Pass (HDR -> LDR)
		RenderToneMapping(context, currentHdrInput);

		// Render LDR Passes
		auto& currentLdrInput = RenderLDRPasses(context, m_LdrBufferA, m_LdrBufferB);
		m_TextureOutputs["FinalScene"] = currentLdrInput->GetColorAttachmentID(0);

		// Blit final LDR result to the screen
		// TODO: Seemingly redundant code with RenderSystem::RenderFinalComposite,
		// May see if I can just set an output buffer
		//BlitToScreen(context, currentLdrInput);
	}

	void PostProcessRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_PostProcessBufferA->ViewportResize(width, height);
		m_PostProcessBufferB->ViewportResize(width, height);
		m_LdrBufferA->ViewportResize(width, height);
		m_LdrBufferB->ViewportResize(width, height);
	}

	void PostProcessRenderPass::Shutdown()
	{
	}

	SharedPtr<Framebuffer>& PostProcessRenderPass::RenderHDRPasses(RenderContext& context, SharedPtr<Framebuffer>& currentHdrInput, SharedPtr<Framebuffer>& currentHdrOutput)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		// Grab outline components for selected entities
		std::unordered_map<EntityID, OutlineComponent> outlinedEntityMap;
		View view = registry.ActiveQuery<OutlineComponent>();
		for (EntityID entity : view)
		{
			auto [outline] = registry.GetComponents<OutlineComponent>(entity);
			outlinedEntityMap[entity] = outline;
		}

		// Process HDR passes, ping-ponging between two buffers.
		// The final output will be in currentHdrInput due to the ping-pong at the end of the loop
		for (auto& pass : m_PostProcessStack)
		{
			if (pass->Enabled && pass->GetStage() == PostProcessStage::HDR)
			{
				// Special Case: Outline Pass needs specific buffers and loops per entity
				if (auto outlinePass = DynamicPointerCast<OutlinePass>(pass))
				{
					for (const auto& [entityID, outline] : outlinedEntityMap)
					{
						outlinePass->SetGBuffer(m_FramebufferInputs["GBuffer"]);
						// MUST pass m_HdrSceneBuffer specifically so it can read Depth and IDs!
						outlinePass->SetHdrBuffer(m_FramebufferInputs["HDRScene"]);
						outlinePass->SetSelectedEntityID(entityID);
						outlinePass->SetOutlineColor(outline.Color);
						outlinePass->SetOutlineThickness(outline.Thickness);

						pass->Render(currentHdrInput, currentHdrOutput);

						// Ping Pong
						currentHdrInput = currentHdrOutput;
						currentHdrOutput = (currentHdrOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
					}
					continue; // Skip the standard render call below
				}

				// Standard HDR Passes (e.g. Bloom)
				pass->Render(currentHdrInput, currentHdrOutput);

				// Ping Pong
				currentHdrInput = currentHdrOutput;
				currentHdrOutput = (currentHdrOutput == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
			}
		}

		return currentHdrInput;
	}

	void PostProcessRenderPass::RenderToneMapping(RenderContext& context, SharedPtr<Framebuffer>& currentHdrInput)
	{
		m_LdrBufferA->Bind();
		RenderAction::SetViewport(context.ViewportDimensions);
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color);

		auto finalShader = Application::Instance().GetAssetManager().GetAsset<Shader>(Constants::Assets::FinalCompositeShad);
		finalShader->Bind();
		finalShader->SetFloat(Constants::Uniforms::Exposure, 1.0f);

		finalShader->SetInt(Constants::Uniforms::Scene, 0);

		RenderAction::SetTextureUnit(0, currentHdrInput->GetColorAttachmentID(0));
		Renderer3D::Submit(m_ScreenQuadVAO);
		m_LdrBufferA->Unbind();
	}

	SharedPtr<Framebuffer>& PostProcessRenderPass::RenderLDRPasses(RenderContext& context, SharedPtr<Framebuffer>& currentLdrInput, SharedPtr<Framebuffer>& currentLdrOutput)
	{
		for (auto& pass : m_PostProcessStack)
		{
			if (pass->Enabled && pass->GetStage() == PostProcessStage::LDR)
			{
				pass->Render(currentLdrInput, currentLdrOutput);

				// Ping Pong
				currentLdrInput = currentLdrOutput;
				currentLdrOutput = (currentLdrOutput == m_LdrBufferA) ? m_LdrBufferB : m_LdrBufferA;
			}
		}

		return currentLdrInput;
	}

	//void PostProcessRenderPass::BlitToScreen(RenderContext& context, SharedPtr<Framebuffer>& currentLdrInput)
	//{
	//	RenderAction::SetFramebuffer(m_FramebufferInputs["OutputFrameBuffer"]);
	//	RenderAction::SetViewport(context.ViewportDimensions);
	//	RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);

	//	m_BlitShader->Bind();

	//	m_BlitShader->SetInt(Constants::Uniforms::Scene, 0);

	//	RenderAction::SetTextureUnit(0, currentLdrInput->GetColorAttachmentID(0));
	//	Renderer3D::Submit(m_ScreenQuadVAO);
	//}

}