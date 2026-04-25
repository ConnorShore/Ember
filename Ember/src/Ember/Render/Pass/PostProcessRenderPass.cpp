#include "ebpch.h"
#include "PostProcessRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/PrimitiveGenerator.h"

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

		auto& assetManager = Application::Instance().GetAssetManager();
		m_BlitShader = assetManager.GetAsset<Shader>(Constants::Assets::BlitShad);
		m_ToneMapShader = assetManager.GetAsset<Shader>(Constants::Assets::ToneMapShadUUID);
	}

	void PostProcessRenderPass::Execute(RenderContext& context)
	{
		PostProcessPassContext passContext(context);

		auto& registry = context.ActiveScene->GetRegistry();
		RenderAction::UseDepthTest(false);

		// Render HDR Passes
		passContext.InputBuffer = m_FramebufferInputs["HDRScene"];
		passContext.OutputBuffer = m_PostProcessBufferA;
		auto currentHdrInput = RenderHDRPasses(passContext);

		// Tone Mapping Pass (HDR -> LDR)
		passContext.InputBuffer = currentHdrInput;
		passContext.OutputBuffer = m_LdrBufferA;
		RenderToneMapping(passContext);

		// Render LDR Passes
		passContext.InputBuffer = m_LdrBufferA;
		passContext.OutputBuffer = m_LdrBufferB;
		auto& currentLdrInput = RenderLDRPasses(passContext);
		m_TextureOutputs["FinalScene"] = currentLdrInput->GetColorAttachmentID(0);
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

	SharedPtr<Framebuffer> PostProcessRenderPass::RenderHDRPasses(PostProcessPassContext& passContext)
	{
		auto& registry = passContext.RenderCtx.ActiveScene->GetRegistry();

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

						pass->Render(passContext);

						// Ping Pong
						passContext.InputBuffer = passContext.OutputBuffer;
						passContext.OutputBuffer = (passContext.OutputBuffer == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
					}
					continue; // Skip the standard render call below
				}

				// Standard HDR Passes (e.g. Bloom)
				pass->Render(passContext);

				// Ping Pong
				passContext.InputBuffer = passContext.OutputBuffer;
				passContext.OutputBuffer = (passContext.OutputBuffer == m_PostProcessBufferA) ? m_PostProcessBufferB : m_PostProcessBufferA;
			}
		}

		return passContext.InputBuffer;
	}

	// TODO: Probably move this to its own pass so it can contain the Exposure setting
	void PostProcessRenderPass::RenderToneMapping(PostProcessPassContext& passContext)
	{
		for (auto& pass : m_PostProcessStack)
		{
			if (pass->GetStage() == PostProcessStage::ToneMap)
			{
				pass->Render(passContext);
				return;
			}
		}
	}

	SharedPtr<Framebuffer>& PostProcessRenderPass::RenderLDRPasses(PostProcessPassContext& passContext)
	{
		for (auto& pass : m_PostProcessStack)
		{
			if (pass->Enabled && pass->GetStage() == PostProcessStage::LDR)
			{
				pass->Render(passContext);

				// Ping Pong
				passContext.InputBuffer = passContext.OutputBuffer;
				passContext.OutputBuffer = (passContext.OutputBuffer == m_LdrBufferA) ? m_LdrBufferB : m_LdrBufferA;
			}
		}

		return passContext.InputBuffer;
	}
}