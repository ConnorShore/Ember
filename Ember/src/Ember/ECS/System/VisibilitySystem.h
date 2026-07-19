#pragma once

#include "System.h"

#include "Ember/ECS/Types.h"
#include "Ember/Render/Frustum.h"

#include <vector>
#include <utility>
#include <unordered_map>

namespace Ember {

	class Scene;

	// Computes a coarse, main-camera-relative visibility set for the active scene once per frame,
	// BEFORE the simulation systems that can use it to skip expensive per-entity CPU work.
	//
	// This is deliberately NOT the same thing as the RenderSystem's frustum culling:
	//   * RenderSystem culls per-view (main viewport, camera preview, and — later — shadow views) at
	//     draw time using the current frame's transforms. That is inherently per-camera and stays in
	//     the RenderSystem.
	//   * VisibilitySystem answers a single question — "is this entity worth simulating this frame?" —
	//     against the one active gameplay camera. It runs before TransformSystem, so it culls against
	//     last frame's transforms; to stay a conservative *superset* of what actually ends up on screen
	//     it dilates the frustum by a world-space margin and keeps a short grace window (hysteresis)
	//     after an entity leaves view.
	//
	// First consumer: AnimationSystem skips the render-only bone/skinning matrix evaluation for
	// off-screen animators while still advancing their state machines, clocks and events. The API is
	// intentionally generic so particle/AI/cloth LOD can consult it later.
	//
	// Fail-safe by design: if no active camera can be resolved (or the system is disabled) every entity
	// reports visible, so we never wrongly freeze something that is actually on screen.
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
