#include "ebpch.h"
#include "ScreenSpace2DRenderPass.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/UILayoutSystem.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Render/RenderAction.h"
#include "Ember/Render/Renderer2D.h"

namespace Ember {

	static bool ShouldRenderScreenSpaceEntity(Scene* scene, EntityID entity, bool drawAll, EntityID selectedEntity)
	{
		if (drawAll)
			return true;

		if (selectedEntity == Constants::Entities::InvalidEntityID)
			return false;

		if (entity == selectedEntity)
			return true;

		Entity selected(selectedEntity, scene);
		if (!selected || !selected.ContainsComponent<IDComponent>())
			return false;

		UUID selectedUUID = selected.GetUUID();
		Entity current(entity, scene);
		while (current && current.ContainsComponent<RelationshipComponent>())
		{
			auto& relationship = current.GetComponent<RelationshipComponent>();
			if (relationship.ParentHandle == Constants::InvalidUUID)
				break;

			if (relationship.ParentHandle == selectedUUID)
				return true;

			current = scene->GetEntity(relationship.ParentHandle);
		}

		return false;
	}


	void ScreenSpace2DRenderPass::Init()
	{
	}

	void ScreenSpace2DRenderPass::Execute(RenderContext& context)
	{
		// UI ignores the 3D world completely
		RenderAction::UseDepthTest(false);
		RenderAction::UseDepthMask(false);
		RenderAction::UseBlending(true);

		// Create an Orthographic matrix perfectly sized to the viewport resolution
		float width = (float)context.ViewportDimensions.z;
		float height = (float)context.ViewportDimensions.w;

		Matrix4f orthoProj = Math::Orthographic(0.0f, width, 0.0f, height, -1.0f, 1.0f);

		// Push the Ortho matrix to the shader's Camera UBO
		context.CameraUniformBuffer->SetData(&orthoProj, sizeof(Matrix4f));

		Renderer2D::BeginFrame();

		// UILayoutSystem already walked the canvases depth-first, so this list is in draw order:
		// canvas SortOrder first, then hierarchy. Depth testing is off, so submission order IS z-order.
		auto uiLayoutSystem = Application::Instance().GetSystemManager().GetSystem<UILayoutSystem>();
		if (uiLayoutSystem)
		{
			auto& registry = context.ActiveScene->GetRegistry();
			for (const UIDrawEntry& entry : uiLayoutSystem->GetSortedScreenSpaceEntities())
			{
				if (!ShouldRenderScreenSpaceEntity(context.ActiveScene, entry.Entity, context.DrawHUD, context.SelectedEntity))
					continue;

				// Sprite before text on the same entity: a button's background must sit under its label.
				if (registry.ContainsComponent<SpriteComponent>(entry.Entity) && registry.ContainsComponent<TransformComponent>(entry.Entity))
					RenderSprite(context.ActiveScene, entry.Entity);

				if (registry.ContainsComponent<TextComponent>(entry.Entity) && registry.ContainsComponent<TransformComponent>(entry.Entity))
					RenderText(context.ActiveScene, entry.Entity);
			}
		}

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

	void ScreenSpace2DRenderPass::RenderSprite(Scene* scene, EntityID entity)
	{
		auto& registry = scene->GetRegistry();
		auto [sprite, transform] = registry.GetComponents<SpriteComponent, TransformComponent>(entity);

		if (sprite.TextureHandle == Constants::InvalidUUID)
		{
			Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color);
			return;
		}

		auto textureAsset = Application::Instance().GetAssetManager().GetAsset<Texture2D>(sprite.TextureHandle);

		bool hasNineSlice = sprite.NineSliceBorder.x > 0.0f || sprite.NineSliceBorder.y > 0.0f
			|| sprite.NineSliceBorder.z > 0.0f || sprite.NineSliceBorder.w > 0.0f;

		if (hasNineSlice)
			Renderer2D::DrawNineSliceQuad(transform.WorldTransform, sprite.Color, textureAsset, sprite.NineSliceBorder, entity);
		else
			Renderer2D::DrawQuad(transform.WorldTransform, sprite.Color, textureAsset);
	}

	void ScreenSpace2DRenderPass::RenderText(Scene* scene, EntityID entity)
	{
		auto& registry = scene->GetRegistry();
		auto [textComp, transform] = registry.GetComponents<TextComponent, TransformComponent>(entity);

		if (textComp.FontHandle == Constants::InvalidUUID || textComp.Text.empty())
			return;

		auto fontAsset = Application::Instance().GetAssetManager().GetAsset<Font>(textComp.FontHandle);
		if (!fontAsset)
			return;

		// A text entity without a RectTransform keeps the legacy behaviour of using its transform directly.
		if (!registry.ContainsComponent<RectTransformComponent>(entity))
		{
			Renderer2D::DrawString(textComp.Text, transform.WorldTransform, textComp.Color, fontAsset, entity, true);
			return;
		}

		auto& rect = registry.GetComponent<RectTransformComponent>(entity);

		// Glyphs are emitted in bake-pixel units, so scale by the authored size rather than the
		// rect's scale - otherwise a label inherits its parent's size and renders enormous.
		float scale = textComp.FontSize / Renderer2D::FontBakePixelHeight;

		Vector2f textMin(0.0f);
		Vector2f textMax(0.0f);
		Renderer2D::MeasureString(textComp.Text, fontAsset, textMin, textMax);
		Vector2f textSize = (textMax - textMin) * scale;
		Vector2f scaledMin = textMin * scale;

		auto alignOffset = [](TextAlignment alignment, float rectExtent, float textExtent, float textMinEdge)
			{
				switch (alignment)
				{
				case TextAlignment::Center: return (rectExtent - textExtent) * 0.5f - textMinEdge;
				case TextAlignment::End:    return rectExtent - textExtent - textMinEdge;
				default:                    return -textMinEdge;
				}
			};

		Vector2f origin = rect.ComputedMin + Vector2f(
			alignOffset(textComp.HorizontalAlignment, rect.ComputedSize.x, textSize.x, scaledMin.x),
			alignOffset(textComp.VerticalAlignment, rect.ComputedSize.y, textSize.y, scaledMin.y));

		Matrix4f textTransform = Math::Translate(Vector3f(origin, 0.0f))
			* Math::GetRotationMatrix(Vector3f(0.0f, 0.0f, rect.Rotation))
			* Math::Scale(Vector3f(scale, scale, 1.0f));

		Renderer2D::DrawString(textComp.Text, textTransform, textComp.Color, fontAsset, entity, true);
	}

}
