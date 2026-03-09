#include "ebpch.h"
#include "RenderSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer.h"

namespace Ember {

	void RenderSystem::OnAttach(Registry* registry)
	{
		EB_CORE_INFO("RenderSystem is attached!");
	}

	void RenderSystem::OnDetach(Registry* registry)
	{
		EB_CORE_INFO("RenderSystem is detached!");
	}

	void RenderSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
		Ember::RenderAction::Clear();

		View cameraView = registry->Query<CameraComponent>();
		for (EntityID cameraEntity : cameraView)
		{
			auto& cameraComp = registry->GetComponent<CameraComponent>(cameraEntity);
			Renderer::BeginFrame(*cameraComp.Camera);
			break;
		}

		View view = registry->Query<SpriteComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [sprite, transform] = registry->GetComponents<SpriteComponent, TransformComponent>(entity);
			Renderer::DrawSprite(sprite, Math::Translate(transform.Transform));
		}

		Ember::Renderer::EndFrame();
	}

}