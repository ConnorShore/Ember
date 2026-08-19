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

	// Unity's ScaleWithScreenSize: authored pixel sizes stay proportional as the viewport changes,
	// so a 200px button is not 200px at both 720p and 4K.
	static float ResolveCanvasScale(const CanvasComponent& canvas, const Vector2f& viewportSize)
	{
		if (canvas.ReferenceResolution.x <= 0.0f || canvas.ReferenceResolution.y <= 0.0f)
			return 1.0f;

		float logWidth = std::log2(viewportSize.x / canvas.ReferenceResolution.x);
		float logHeight = std::log2(viewportSize.y / canvas.ReferenceResolution.y);
		float match = std::clamp(canvas.MatchWidthOrHeight, 0.0f, 1.0f);
		return std::pow(2.0f, logWidth + (logHeight - logWidth) * match);
	}

	static UILayoutRect ResolveRectTransform(RectTransformComponent& rect, TransformComponent& transform, const UILayoutRect& parentRect, float canvasScale)
	{
		Vector2f anchorMin = parentRect.Min + parentRect.Size * rect.AnchorMin;
		Vector2f anchorMax = parentRect.Min + parentRect.Size * rect.AnchorMax;
		Vector2f anchorSize = anchorMax - anchorMin;

		// Authored pixel offsets scale with the canvas; anchors and pivot are unitless and do not.
		Vector2f scaledSizeDelta = rect.SizeDelta * canvasScale;
		Vector2f scaledAnchoredPosition = rect.AnchoredPosition * canvasScale;

		Vector2f size = anchorSize + scaledSizeDelta;
		Vector2f pivotPosition = anchorMin + anchorSize * rect.Pivot + scaledAnchoredPosition;
		Vector2f centerPosition = pivotPosition + (Vector2f(0.5f) - rect.Pivot) * size;

		transform.WorldTransform = Math::Translate(Vector3f(centerPosition, 0.0f))
			* Math::GetRotationMatrix(Vector3f(0.0f, 0.0f, rect.Rotation))
			* Math::Scale(Vector3f(size, 1.0f));

		UILayoutRect resolvedRect;
		resolvedRect.Min = pivotPosition - rect.Pivot * size;
		resolvedRect.Size = size;

		// Cached so UIInputSystem can hit-test without recomputing the layout.
		rect.ComputedMin = resolvedRect.Min;
		rect.ComputedSize = resolvedRect.Size;

		return resolvedRect;
	}

	static void ResolveChildren(Scene* scene, EntityID parentEntity, const UILayoutRect& parentRect, float canvasScale,
		uint32_t canvasSortOrder, std::vector<UIDrawEntry>& outEntries)
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

			// Skipping the whole subtree is deliberate: disabling a panel must hide its contents too.
			if (registry.ContainsComponent<DisabledComponent>(childEntity))
				continue;

			UILayoutRect childRect = parentRect;
			if (registry.ContainsComponent<RectTransformComponent>(childEntity) && registry.ContainsComponent<TransformComponent>(childEntity))
			{
				auto [rect, transform] = registry.GetComponents<RectTransformComponent, TransformComponent>(childEntity);
				childRect = ResolveRectTransform(rect, transform, parentRect, canvasScale);
			}

			UIDrawEntry entry;
			entry.Entity = childEntity;
			entry.CanvasSortOrder = canvasSortOrder;
			entry.HierarchyIndex = (uint32_t)outEntries.size();
			outEntries.push_back(entry);

			ResolveChildren(scene, childEntity, childRect, canvasScale, canvasSortOrder, outEntries);
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
		m_SortedEntities.clear();

		// Must match the source ScreenSpace2DRenderPass builds its orthographic projection from,
		// or layout and rendering disagree. Scene::GetViewportSize proxies the active camera and
		// is only a fallback.
		int viewportDims[4] = { 0 };
		RenderAction::GetViewportDimensions(viewportDims);
		Vector2f viewportSize = Vector2f((float)viewportDims[2], (float)viewportDims[3]);
		if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
			viewportSize = scene->GetViewportSize();
		if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
			return;

		auto& registry = scene->GetRegistry();

		// Visit canvases in SortOrder so the draw list is already layered correctly.
		std::vector<EntityID> canvases;
		for (auto entity : registry.ActiveQuery<CanvasComponent, TransformComponent>())
		{
			if (registry.GetComponent<CanvasComponent>(entity).RenderMode == CanvasRenderMode::ScreenSpace)
				canvases.push_back(entity);
		}

		std::stable_sort(canvases.begin(), canvases.end(), [&registry](EntityID a, EntityID b)
			{
				return registry.GetComponent<CanvasComponent>(a).SortOrder < registry.GetComponent<CanvasComponent>(b).SortOrder;
			});

		for (EntityID entity : canvases)
		{
			auto& canvasComp = registry.GetComponent<CanvasComponent>(entity);

			UILayoutRect canvasRect;
			canvasRect.Min = Vector2f(0.0f);
			canvasRect.Size = viewportSize;

			// The canvas entity itself is a pure container - its own WorldTransform is a stale 3D
			// transform that would render as garbage under the UI's orthographic projection.
			ResolveChildren(scene, entity, canvasRect, ResolveCanvasScale(canvasComp, viewportSize),
				canvasComp.SortOrder, m_SortedEntities);
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
