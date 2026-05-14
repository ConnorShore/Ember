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
		RenderSprites(registry, context.DrawHUD, context.SelectedEntity, width, height);

		// Draw Screen-Space Text (e.g. Ammo, Health)
		RenderText(registry, context.DrawHUD, context.SelectedEntity, width, height);

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

	void ScreenSpace2DRenderPass::RenderSprites(Registry& registry, bool drawAll, EntityID selectedEntity, float viewportWidth, float viewportHeight)
	{
		for (EntityID entity : registry.ActiveQuery<SpriteComponent, TransformComponent>())
		{
			if (!drawAll && entity != selectedEntity)
				continue;

			auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);

			// Treat transform translation X/Y as normalized [0, 1] viewport coords so
			// UI stays in the same relative position when the window is resized.
			Matrix4f screenTransform = transform.WorldTransform;
			screenTransform[3][0] *= viewportWidth;
			screenTransform[3][1] *= viewportHeight;

			if (sprite.TextureHandle == Constants::InvalidUUID)
			{
				Renderer2D::DrawQuad(screenTransform, sprite.Color);
			}
			else
			{
				auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture2D>(sprite.TextureHandle);
				Renderer2D::DrawQuad(screenTransform, sprite.Color, textureAsset);
			}
		}
	}

	void ScreenSpace2DRenderPass::RenderText(Registry& registry, bool drawAll, EntityID selectedEntity, float viewportWidth, float viewportHeight)
	{
		for (EntityID entity : registry.ActiveQuery<TextComponent, TransformComponent>())
		{
			if (!drawAll && entity != selectedEntity)
				continue;

			auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);
			if (textComp.ScreenSpace && textComp.FontHandle != Constants::InvalidUUID && !textComp.Text.empty())
			{
				auto fontAsset = Application::Instance().GetAssetManager().GetAsset<Font>(textComp.FontHandle);
				if (fontAsset)
				{
					// Treat transform translation X/Y as normalized [0, 1] viewport coords so
					// UI stays in the same relative position when the window is resized.
					Matrix4f screenTransform = transform.WorldTransform;
					screenTransform[3][0] *= viewportWidth;
					screenTransform[3][1] *= viewportHeight;

					Renderer2D::DrawString(textComp.Text, screenTransform, textComp.Color, fontAsset, entity, true);
				}
			}
		}
	}

}