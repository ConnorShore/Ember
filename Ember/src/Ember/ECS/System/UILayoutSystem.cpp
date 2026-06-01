#include "ebpch.h"
#include "UILayoutSystem.h"

#include "Ember/Scene/Scene.h"
#include "Ember/Render/RenderAction.h"

namespace Ember {

	struct UILayoutRect
	{
		Vector2f Min = Vector2f(0.0f);
		Vector2f Size = Vector2f(0.0f);
	};

	static UILayoutRect ResolveRectTransform(const RectTransformComponent& rect, TransformComponent& transform, const UILayoutRect& parentRect)
	{
		Vector2f anchorMin = parentRect.Min + parentRect.Size * rect.AnchorMin;
		Vector2f anchorMax = parentRect.Min + parentRect.Size * rect.AnchorMax;
		Vector2f anchorSize = anchorMax - anchorMin;

		Vector2f size = anchorSize + rect.SizeDelta;
		Vector2f pivotPosition = anchorMin + anchorSize * rect.Pivot + rect.AnchoredPosition;
		Vector2f centerPosition = pivotPosition + (Vector2f(0.5f) - rect.Pivot) * size;

		transform.WorldTransform = Math::Translate(Vector3f(centerPosition, 0.0f))
			* Math::GetRotationMatrix(Vector3f(0.0f, 0.0f, rect.Rotation))
			* Math::Scale(Vector3f(size, 1.0f));

		UILayoutRect resolvedRect;
		resolvedRect.Min = pivotPosition - rect.Pivot * size;
		resolvedRect.Size = size;
		return resolvedRect;
	}

	static void ResolveChildren(Scene* scene, EntityID parentEntity, const UILayoutRect& parentRect)
	{
		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<RelationshipComponent>(parentEntity))
			return;

		auto& relationship = registry.GetComponent<RelationshipComponent>(parentEntity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child == Constants::Entities::InvalidEntityID)
				continue;

			EntityID childEntity = child.GetEntityHandle();
			if (registry.ContainsComponent<DisabledComponent>(childEntity))
				continue;

			UILayoutRect childRect = parentRect;
			if (registry.ContainsComponent<RectTransformComponent>(childEntity) && registry.ContainsComponent<TransformComponent>(childEntity))
			{
				auto [rect, transform] = registry.GetComponents<RectTransformComponent, TransformComponent>(childEntity);
				childRect = ResolveRectTransform(rect, transform, parentRect);
			}

			ResolveChildren(scene, childEntity, childRect);
		}
	}

	void UILayoutSystem::OnAttach()
	{
		EB_CORE_INFO("UILayoutSystem is attached!");
	}

	void UILayoutSystem::OnDetach()
	{
		EB_CORE_INFO("UILayoutSystem is detached!");
	}

	void UILayoutSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		int viewportDims[4] = { 0 };
		RenderAction::GetViewportDimensions(viewportDims);
		Vector2f viewportSize = Vector2f((float)viewportDims[2], (float)viewportDims[3]);
		if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
			viewportSize = scene->GetViewportSize();
		if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
			return;

		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<CanvasComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto& canvasComp = registry.GetComponent<CanvasComponent>(entity);
			if (canvasComp.RenderMode != CanvasRenderMode::ScreenSpace)
				continue;

			UILayoutRect canvasRect;
			canvasRect.Min = Vector2f(0.0f);
			canvasRect.Size = viewportSize;
			ResolveChildren(scene, entity, canvasRect);
		}
	}

	void UILayoutSystem::OnViewportResize(Scene* scene, uint32_t width, uint32_t height)
	{
		// Mark all canvas entities as dirty so they will recalculate their layout on the next update
		auto view = scene->GetRegistry().ActiveQuery<CanvasComponent>();
		for (auto entity : view)
		{
			auto& canvas = scene->GetRegistry().GetComponent<CanvasComponent>(entity);
			canvas.IsDirty = true;
		}
	}

}