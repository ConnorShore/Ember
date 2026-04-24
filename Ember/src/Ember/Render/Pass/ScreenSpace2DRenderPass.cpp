#include "ebpch.h"
#include "ScreenSpace2DRenderPass.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"

namespace Ember {


	void ScreenSpace2DRenderPass::Init()
	{
	}

	void ScreenSpace2DRenderPass::Execute(RenderContext& context)
	{
		auto& registry = context.ActiveScene->GetRegistry();

		// Bind the final screen/editor output target
		//RenderAction::SetFramebuffer(m_FramebufferInputs["OutputFrameBuffer"]);

		// UI ignores the 3D world completely
		RenderAction::UseDepthTest(false);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);

		// Create an Orthographic matrix perfectly sized to the viewport resolution
		float width = (float)context.ViewportDimensions.z;
		float height = (float)context.ViewportDimensions.w;

		// Note: Depending on your OpenGL setup, you may need to invert the top/bottom 
		// if your UI renders upside down.
		Matrix4f orthoProj = Math::Orthographic(0.0f, width, 0.0f, height, -1.0f, 1.0f);

		// Push the Ortho matrix to the shader's Camera UBO
		context.CameraUniformBuffer->SetData(&orthoProj, sizeof(Matrix4f));

		Renderer2D::BeginFrame();

		// Draw Screen-Space Sprites (e.g. Crosshairs, Minimaps)
		RenderSprites(registry);

		// Draw Screen-Space Text (e.g. Ammo, Health)
		RenderText(registry);

		Renderer2D::EndFrame();

		// Put the 3D ViewProjection matrix back in the UBO so the next frame starts correctly
		Matrix4f viewProjectionMat = context.ActiveCamera->GetProjectionMatrix() * Math::Inverse(context.CameraTransform);
		context.CameraUniformBuffer->SetData(&viewProjectionMat, sizeof(Matrix4f));

		RenderAction::UseDepthTest(true);
		RenderAction::UseDepthMask(true);
	}

	void ScreenSpace2DRenderPass::OnViewportResize(uint32_t width, uint32_t height)
	{

	}

	void ScreenSpace2DRenderPass::Shutdown()
	{
	}

	void ScreenSpace2DRenderPass::RenderSprites(Registry& registry)
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
				auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture2D>(sprite.TextureHandle);
				Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color, textureAsset);
			}
		}
	}

	void ScreenSpace2DRenderPass::RenderText(Registry& registry)
	{
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);
			if (textComp.ScreenSpace && textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = Application::Instance().GetAssetManager().GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
					Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, true);
			}
		}
	}

}