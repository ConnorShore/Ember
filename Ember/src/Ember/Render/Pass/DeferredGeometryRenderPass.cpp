#include "ebpch.h"
#include "DeferredGeometryRenderPass.h"

#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/Framebuffer.h"

namespace Ember {

	void DeferredGeometryRenderPass::Init()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		m_DefaultWhite = assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultWhiteTex);
		m_DefaultBlack = assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultBlackTex);
		m_DefaultNormal = assetManager.GetAsset<Texture2D>(Constants::Assets::DefaultNormalTex);

		// GBuffer: packed deferred rendering targets
		// [0] AlbedoRoughness  [1] NormalMetallic  [2] PositionAO
		// [3] Emission  [4] EntityID (integer for picking)  [5] Depth
		Ember::FramebufferSpecification specs;
		specs.Width = 1;
		specs.Height = 1;
		specs.AttachmentSpecs = {
			Ember::FramebufferTextureFormat::RGBA8,				// AlbedoRoughness
			Ember::FramebufferTextureFormat::RGBA16F,			// NormalMetallic
			Ember::FramebufferTextureFormat::RGBA16F,			// PositionAO
			Ember::FramebufferTextureFormat::RGBA16F,			// Emission
			Ember::FramebufferTextureFormat::RedInteger,		// EntityID
			Ember::FramebufferTextureFormat::Depth24Stencil8	// Depth
		};
		m_GBuffer = Framebuffer::Create(specs);
		m_FramebufferOutputs["GBuffer"] = m_GBuffer;

		m_TextureOutputs["AlbedoRoughness"] = m_GBuffer->GetColorAttachmentID(0);
		m_TextureOutputs["NormalMetallic"] = m_GBuffer->GetColorAttachmentID(1);
		m_TextureOutputs["PositionAO"] = m_GBuffer->GetColorAttachmentID(2);
		m_TextureOutputs["Emission"] = m_GBuffer->GetColorAttachmentID(3);
		m_TextureOutputs["EntityID"] = m_GBuffer->GetColorAttachmentID(4);
		m_TextureOutputs["Depth"] = m_GBuffer->GetDepthAttachmentID();
	}

	void DeferredGeometryRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		m_GBuffer->Bind();
		RenderAction::SetViewport(0, 0, m_GBuffer->GetSpecification().Width, m_GBuffer->GetSpecification().Height);

		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));
		RenderAction::Clear(Ember::RendererAPI::RenderBit::Color | Ember::RendererAPI::RenderBit::Depth);
		RenderAction::UseDepthTest(true);

		// Clear EntityId attachment
		int clearValue = Constants::Entities::InvalidEntityID;
		m_GBuffer->ClearAttachment(4, clearValue);

		// Bind default white as the default texture for all units to avoid accidentally sampling from unbound texture units in the shader
		RenderAction::SetTextureUnit(0, m_DefaultWhite->GetID());
		RenderAction::SetTextureUnit(1, m_DefaultNormal->GetID());
		RenderAction::SetTextureUnit(2, m_DefaultWhite->GetID());
		RenderAction::SetTextureUnit(3, m_DefaultBlack->GetID());

		Renderer3D::BeginFrame();

		for (EntityID entity : context.RenderQueueBuckets->Opaque)
		{
			auto [material, transform] = registry.GetComponents<MaterialComponent, TransformComponent>(entity);
			if (material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			materialAsset->GetShader()->Bind();
			materialAsset->GetShader()->SetInt(Constants::Uniforms::EntityID, entity);

			if (registry.ContainsComponent<StaticMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<StaticMeshComponent>(entity);
				if (mesh.MeshHandle == Constants::InvalidUUID)
					continue;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
			else if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				if (mesh.MeshHandle == Constants::InvalidUUID)
					continue;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);

				// Cache runtime animator Id -> do expensive lookup only once
				if (mesh.RuntimeAnimatorID == Constants::Entities::InvalidEntityID && mesh.AnimatorEntityHandle != Constants::InvalidUUID)
				{
					Entity animatorEnt = context.ActiveScene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEnt.GetEntityHandle() != Constants::Entities::InvalidEntityID)
					{
						mesh.RuntimeAnimatorID = animatorEnt.GetEntityHandle();
					}
				}

				// Use cached runtime animator id
				if (mesh.RuntimeAnimatorID != Constants::Entities::InvalidEntityID)
				{
					EB_CORE_ASSERT(registry.ContainsComponent<AnimatorComponent>(mesh.RuntimeAnimatorID), "Animator component should be present");

					auto& animator = registry.GetComponent<AnimatorComponent>(mesh.RuntimeAnimatorID);
					materialAsset->GetShader()->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
				}

				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
		}

		Renderer3D::EndFrame();
	}

	void DeferredGeometryRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_GBuffer->ViewportResize(width, height);
	}

	void DeferredGeometryRenderPass::Shutdown()
	{
	}

}