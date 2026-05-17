#include "ebpch.h"
#include "TransparentEntitiesRenderPass.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer3D.h"
#include "Ember/Render/Framebuffer.h"

namespace Ember {

	void TransparentEntitiesRenderPass::Init()
	{
	}

	void TransparentEntitiesRenderPass::Execute(RenderContext& context)
	{
		// If there are no transparent entities, skip the pass entirely
		if (context.RenderQueueBuckets->Transparent.empty())
			return;

		auto& registry = context.ActiveScene->GetRegistry();

		m_FramebufferInputs["HDRScene"]->Bind();

		Renderer3D::BeginFrame();

		// 1. STATE SETUP FOR TRANSPARENCY
		// Note: must be set AFTER BeginFrame(), which resets blending to false
		RenderAction::UseDepthTest(true);    // We still want to hide glass behind brick walls
		RenderAction::UseDepthMask(false);   // BUT we don't want glass to block other glass
		RenderAction::UseBlending(true);     // Turn on Alpha Blending

		// 2. DEPTH SORTING (Back to Front)
		// We make a copy of the transparent bucket so we can sort it
		std::vector<EntityID> sortedEntities = context.RenderQueueBuckets->Transparent;
		Vector3f cameraPos = Vector3f(context.CameraTransform[3]); // Extract position from camera matrix

		std::sort(sortedEntities.begin(), sortedEntities.end(), [&](EntityID a, EntityID b) {
			auto& transformA = registry.GetComponent<TransformComponent>(a);
			auto& transformB = registry.GetComponent<TransformComponent>(b);

			// Calculate squared distance (faster than actual distance since we just need relative order)
			float distA = Math::Distance2(transformA.GetWorldPosition(), cameraPos);
			float distB = Math::Distance2(transformB.GetWorldPosition(), cameraPos);

			// Sort descending (Furthest objects get drawn first)
			return distA > distB;
			});

		// 3. RENDER LOOP (Identical to Forward Pass)
		for (EntityID entity : sortedEntities)
		{
			auto [material, transform] = registry.GetComponents<MaterialComponent, TransformComponent>(entity);
			if (material.MaterialHandle == Constants::InvalidUUID)
				continue;

			auto materialAsset = Application::Instance().GetAssetManager().GetAsset<MaterialBase>(material.MaterialHandle);
			if (!materialAsset->GetShader())
				continue;

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

		Renderer3D::EndFrame();

		RenderAction::UseDepthMask(true);

		m_FramebufferInputs["HDRScene"]->Unbind();
	}

	void TransparentEntitiesRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

	void TransparentEntitiesRenderPass::Shutdown()
	{
	}

}