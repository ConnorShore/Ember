#include "ebpch.h"
#include "WorldSpace2DRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"

namespace Ember {
	static Entity FindNearestCanvasAncestor(Scene* scene, Entity entity)
	{
		Entity current = entity;
		while (current)
		{
			if (current.ContainsComponent<CanvasComponent>())
				return current;

			auto& relationship = current.GetComponent<RelationshipComponent>();
			if (relationship.ParentHandle == Constants::InvalidUUID)
				break;

			current = scene->GetEntity(relationship.ParentHandle);
		}

		return Entity();
	}

	void WorldSpace2DRenderPass::Init()
	{
	}

	void WorldSpace2DRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		m_FramebufferInputs["HDRScene"]->Bind();

		// Enable depth testing so walls hide text, but disable writing so text doesn't cut holes
		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);

		Renderer2D::SetBillboardCameraData(
			Vector3f(context.CameraTransform[3]),
			Vector3f(context.CameraTransform[0]),
			Vector3f(context.CameraTransform[1])
		);

		Renderer2D::BeginFrame();

		// Draw World-Space Sprites and Text
		auto& assetManager = Application::Instance().GetAssetManager();
		DrawSprites(assetManager, context.ActiveScene);
		DrawText(assetManager, context.ActiveScene);

		Renderer2D::EndFrame();

		RenderAction::UseDepthMask(true);

		m_FramebufferInputs["HDRScene"]->Unbind();
	}

	void WorldSpace2DRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void WorldSpace2DRenderPass::Shutdown()
	{
	}

	void WorldSpace2DRenderPass::DrawSprites(AssetManager& assetManager, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		for (EntityID entity : registry.ActiveQuery<SpriteComponent, TransformComponent>())
		{
			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);

			// Verify this text entity is a descendant of a World Space canvas
			auto canvas = FindNearestCanvasAncestor(scene, Entity(entity, scene));
			bool isScreenSpace = (canvas != Constants::Entities::InvalidEntityID) && canvas.GetComponent<CanvasComponent>().RenderMode == CanvasRenderMode::ScreenSpace;
			if (isScreenSpace)
				continue;

			if (sprite.IsBillboard)
			{
				Vector3f worldPos, worldRot, worldScale;
				Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);
				(void)worldRot;

				Vector2f quadSize = {
					std::abs(worldScale.x),
					std::abs(worldScale.y)
				};

				if (sprite.TextureHandle == Constants::InvalidUUID)
				{
					Renderer2D::DrawBillboardQuad(worldPos, quadSize, sprite.Color, sprite.LockYAxis);
				}
				else
				{
					auto textureAsset = assetManager.GetAsset<Texture2D>(sprite.TextureHandle);
					Renderer2D::DrawBillboardQuad(worldPos, quadSize, sprite.Color, textureAsset, sprite.LockYAxis);
				}

				continue;
			}

			if (sprite.TextureHandle == Constants::InvalidUUID)
			{
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color);
			}
			else
			{
				auto textureAsset = assetManager.GetAsset<Texture2D>(sprite.TextureHandle);
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color, textureAsset);
			}
		}

	}

	void WorldSpace2DRenderPass::DrawText(AssetManager& assetManager, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Draw World-Space Text
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);

			// Verify this text entity is a descendant of a World Space canvas
			auto canvas = FindNearestCanvasAncestor(scene, Entity(entity, scene));
			bool isScreenSpace = (canvas != Constants::Entities::InvalidEntityID) && canvas.GetComponent<CanvasComponent>().RenderMode == CanvasRenderMode::ScreenSpace;
			if (isScreenSpace)
				continue;

			if (textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = assetManager.GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
					Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, false);
			}
		}
	}

}