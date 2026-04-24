#include "ebpch.h"
#include "WorldSpace2DRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/Framebuffer.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"

namespace Ember {

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

		Renderer2D::BeginFrame();

		// Draw World-Space Sprites and Text
		auto& assetManager = Application::Instance().GetAssetManager();
		DrawSprites(assetManager, registry);
		DrawText(assetManager, registry);

		Renderer2D::EndFrame();

		RenderAction::UseDepthMask(true);

		m_FramebufferInputs["HDRScene"]->Unbind();
	}

	void WorldSpace2DRenderPass::Shutdown()
	{
	}

	void WorldSpace2DRenderPass::DrawSprites(AssetManager& assetManager, Registry& registry)
	{
		for (EntityID entity : registry.ActiveQuery<SpriteComponent, TransformComponent>())
		{
			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);
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

	void WorldSpace2DRenderPass::DrawText(AssetManager& assetManager, Registry& registry)
	{
		// Draw World-Space Text
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);
			if (!textComp.ScreenSpace && textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = assetManager.GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
					Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, false);
			}
		}
	}

}