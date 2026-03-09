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
		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		RenderAction::Clear();

		// TODO: 3d Render pass


		// 2d Render Pass
		bool beginSceneCalled = false;
		View cameraView = registry->Query<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry->GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				Matrix4f transformMat = Math::Inverse(Math::Translate(transform.Position));
				Renderer2D::BeginFrame(camera, transformMat);
				beginSceneCalled = true;
				break;
			}
		}

		if (!beginSceneCalled)
		{
			EB_CORE_WARN("No camera found! Scene not rendering");
			return;
		}

		{

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