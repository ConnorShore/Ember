#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"
#include "Ember/Render/Renderer3D.h"

namespace Ember {

	void RenderSystem::OnAttach(Registry* registry)
	{
		Renderer2D::Init();
		Renderer3D::Init();
		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach(Registry* registry)
	{
		Renderer2D::Shutdown();
		Renderer3D::Shutdown();
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		RenderAction::Clear();

		bool cameraFound = false;
		CameraComponent activeCamera;
		Matrix4f cameraTransformMatrix;
		View cameraView = registry->Query<CameraComponent, TransformComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto [camera, transform] = registry->GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (camera.IsActive)
			{
				activeCamera = camera;
				cameraTransformMatrix = Math::Translate(transform.Position) * Math::GetRotationMatrix(transform.Rotation);
				cameraFound = true;
				break;
			}
		}

		if (!cameraFound)
		{
			EB_CORE_WARN("No camera found! Scene not rendering");
			return;
		}

		// 3D Render pass
		{
			Renderer3D::BeginFrame(activeCamera, cameraTransformMatrix);

			// Grab lights (max 4 right now till deferred shading is added)
			std::array<std::tuple<PointLightComponent, TransformComponent>, 4> lights;
			unsigned int ct = 0;
			View lightView = registry->Query<PointLightComponent, TransformComponent>();
			for (EntityID entity : lightView)
			{
				if (ct >= 4)
					break;

				auto [light, transform] = registry->GetComponents<PointLightComponent, TransformComponent>(entity);
				lights[ct++] = std::make_tuple(light, transform);
			}

			// Render meshes
			View view = registry->Query<MeshComponent, MaterialComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				auto [mesh, material, transform] = registry->GetComponents<MeshComponent, MaterialComponent, TransformComponent>(entity);
				Renderer3D::Submit(mesh.Mesh->GetVertexArray(), material, transform.GetTransformationMatrix(), lights);
			}

			Renderer3D::EndFrame();
		}


		// 2D Render Pass
		{
			Renderer2D::BeginFrame(activeCamera, cameraTransformMatrix);

			View view = registry->Query<SpriteComponent, TransformComponent>();
			for (EntityID entity : view)
			{
				auto [sprite, transform] = registry->GetComponents<SpriteComponent, TransformComponent>(entity);
				if (sprite.Texture == nullptr)
					Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
						Vector2f(transform.Size.x, transform.Size.y), sprite.Color);
				else
					Renderer2D::DrawQuad(Vector2f(transform.Position.x, transform.Position.y),
					Vector2f(transform.Size.x, transform.Size.y), sprite.Color, sprite.Texture);
			}

			Renderer2D::EndFrame();
		}
	}

}