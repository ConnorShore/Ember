#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"

namespace Ember {

	void RenderSystem::OnAttach(Registry* registry)
	{
		Renderer2D::Init();
		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach(Registry* registry)
	{
		Renderer2D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		Ember::RenderAction::Clear();

		// TODO: 3d Render pass


		// 2d Render Pass
		OrthographicCamera* orthoCamera = nullptr;
		View cameraView = registry->Query<CameraComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto& cameraComp = registry->GetComponent<CameraComponent>(cameraEntity);
			if (!cameraComp.IsPerspective)
			{
				orthoCamera = (OrthographicCamera*)cameraComp.Camera;
				break;
			}
		}
		EB_CORE_ASSERT(orthoCamera, "No ortho camera found for 2D Renderer!");

		{
			Renderer2D::BeginFrame(*orthoCamera);

			View view = registry->Query<SpriteComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				auto [sprite, transform] = registry->GetComponents<SpriteComponent, TransformComponent>(entity);
				Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Size.x, transform.Size.y), sprite.Color);
			}

			Renderer2D::EndFrame();
		}
	}

}