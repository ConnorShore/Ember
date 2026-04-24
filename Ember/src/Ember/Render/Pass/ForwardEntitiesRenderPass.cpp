#include "ebpch.h"
#include "ForwardEntitiesRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/Framebuffer.h"

namespace Ember {


	void ForwardEntitiesRenderPass::Init()
	{
	}

	void ForwardEntitiesRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		m_FramebufferInputs["HDRScene"]->Bind();

		RenderAction::UseDepthTest(true);

		Renderer3D::BeginFrame();

		for (EntityID entity : context.RenderQueueBuckets->Forward)
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
				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
			else if (registry.ContainsComponent<SkinnedMeshComponent>(entity))
			{
				auto& mesh = registry.GetComponent<SkinnedMeshComponent>(entity);
				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);

				if (mesh.AnimatorEntityHandle != Constants::InvalidUUID && context.ActiveScene)
				{
					Entity animatorEntity = context.ActiveScene->GetEntity(mesh.AnimatorEntityHandle);
					if (animatorEntity.GetEntityHandle() != Constants::Entities::InvalidEntityID && registry.ContainsComponent<AnimatorComponent>(animatorEntity.GetEntityHandle()))
					{
						auto& animator = registry.GetComponent<AnimatorComponent>(animatorEntity.GetEntityHandle());
						materialAsset->GetShader()->SetMatrix4Array(Constants::Uniforms::BoneMatrices, animator.BoneMatrices.data(), static_cast<uint32_t>(animator.BoneMatrices.size()));
					}
				}
				Renderer3D::Submit(meshAsset->GetVertexArray(), materialAsset, transform.WorldTransform);
			}
		}

		m_FramebufferInputs["HDRScene"]->Unbind();

		Renderer3D::EndFrame();
	}

	void ForwardEntitiesRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void ForwardEntitiesRenderPass::Shutdown()
	{
	}

}