#pragma once

#include "System.h"

#include "Ember/ECS/Types.h"
#include "Ember/Render/Frustum.h"

#include <vector>
#include <utility>
#include <unordered_map>

namespace Ember {

	class Scene;

	// Coarse, camera-relative "is this entity worth simulating this frame?" set, computed once per frame
	// before the systems that consume it - distinct from RenderSystem's per-view draw-time culling.
	// It runs before TransformSystem, so it dilates the frustum and keeps a short grace window to stay a
	// conservative superset; with no active camera every entity reports visible (fail-safe).
	class VisibilitySystem : public System
	{
	public:
		VisibilitySystem() = default;
		virtual ~VisibilitySystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnSceneDetach(Scene* scene) override;

		void OnUpdate(TimeStep delta, Scene* scene) override;

		// True if the entity passed the (dilated) frustum test on the current frame.
		bool IsVisible(EntityID entity) const { return WasVisibleWithin(entity, 0); }

		// True if the entity was visible within the last `graceFrames` frames. A small grace window
		// avoids tearing expensive work on/off as an entity flickers across the frustum boundary.
		bool WasVisibleWithin(EntityID entity, uint32_t graceFrames) const;

		// Convenience for the common "should I still simulate this?" case, using the configured window.
		bool IsRelevant(EntityID entity) const { return WasVisibleWithin(entity, m_GraceFrames); }

		void SetEnabled(bool enabled) { m_Enabled = enabled; }
		bool IsEnabled() const { return m_Enabled; }

		void SetFrustumMargin(float margin) { m_FrustumMargin = margin; }
		float GetFrustumMargin() const { return m_FrustumMargin; }

		void SetGraceFrames(uint32_t frames) { m_GraceFrames = frames; }
		uint32_t GetGraceFrames() const { return m_GraceFrames; }

		// Shared with RenderSystem so both derive renderable world-AABBs from the exact same bounds
		// math (bind-pose bounds x world transform; skinned entities union the mesh + animator roots).
		// Fills `outEntities` (cleared first) using whatever world transforms are current at call time.
		static void GatherRenderableAABBs(Scene* scene, std::vector<std::pair<EntityID, AABB>>& outEntities);

		// The world AABB a single entity would be culled with; false if it draws no geometry. Shared
		// with the editor so framing a selection uses the same bounds the renderer does.
		static bool TryGetRenderableAABB(Scene* scene, EntityID entity, AABB& outAABB);

	private:
		// Stamp `entity` as visible on the current frame.
		void MarkVisible(EntityID entity);

	private:
		bool m_Enabled = true;

		// World-space distance the frustum planes are pushed outward before testing. Absorbs the
		// one-frame transform lag (this system runs before TransformSystem) plus fast camera motion,
		// keeping the visible set a conservative superset of what the RenderSystem will actually draw.
		float m_FrustumMargin = 2.0f;

		// Entities stay "relevant" for this many frames after they were last visible.
		uint32_t m_GraceFrames = 10;

		// Monotonic frame counter; paired with m_LastVisibleFrame for grace-window checks.
		uint64_t m_FrameIndex = 0;

		// True once the current frame produced a cull against a valid camera. While false, everything
		// reports visible (fail-safe).
		bool m_HasCullData = false;

		// entity -> the last frame index on which it (or a mesh it drives) passed the frustum test.
		std::unordered_map<EntityID, uint64_t> m_LastVisibleFrame;

		// Reused scratch to avoid per-frame allocation, mirroring RenderSystem::m_ActiveRenderableEntities.
		std::vector<std::pair<EntityID, AABB>> m_RenderableScratch;
	};

}
