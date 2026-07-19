#include "ebpch.h"

#include "VisibilitySystem.h"

#include "Ember/Core/Application.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/Entity.h"

#include "Ember/ECS/Registry.h"
#include "Ember/ECS/Component/Components.h"

#include "Ember/Render/Mesh.h"

#include <limits>

namespace Ember {

	// Resolves the active gameplay camera's view-projection matrix, mirroring RenderSystem::SetSceneCamera
	// so the relevance cull matches the camera the scene is actually rendered through at runtime.
	static bool TryGetActiveCameraViewProjection(Scene* scene, Matrix4f& outViewProjection)
	{
		auto& registry = scene->GetRegistry();
		for (EntityID cameraEntity : registry.ActiveQuery<CameraComponent, TransformComponent>())
		{
			auto [camera, transform] = registry.GetComponents<CameraComponent, TransformComponent>(cameraEntity);
			if (!camera.IsActive)
				continue;

			outViewProjection = camera.Camera.GetProjectionMatrix() * Math::Inverse(transform.WorldTransform);
			return true;
		}
		return false;
	}

	// Resolves the animator entity that drives a skinned mesh, mirroring BoneSocketSystem's resolution
	// (fast RuntimeAnimatorID cache, falling back to the serialized AnimatorEntityHandle UUID).
	static EntityID ResolveAnimatorEntityID(Scene* scene, EntityID meshEntity)
	{
		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<SkinnedMeshComponent>(meshEntity))
			return Constants::Entities::InvalidEntityID;

		auto& skinnedMesh = registry.GetComponent<SkinnedMeshComponent>(meshEntity);
		if (skinnedMesh.RuntimeAnimatorID != Constants::Entities::InvalidEntityID &&
			registry.ContainsComponent<AnimatorComponent>(skinnedMesh.RuntimeAnimatorID))
			return skinnedMesh.RuntimeAnimatorID;

		Entity animatorEntity = scene->GetEntity(skinnedMesh.AnimatorEntityHandle);
		if (animatorEntity && animatorEntity.ContainsComponent<AnimatorComponent>())
		{
			skinnedMesh.RuntimeAnimatorID = animatorEntity.GetEntityHandle(); // populate the cache
			return animatorEntity.GetEntityHandle();
		}

		return Constants::Entities::InvalidEntityID;
	}

	void VisibilitySystem::OnAttach()
	{
		EB_CORE_INFO("Visibility System attached!");
	}

	void VisibilitySystem::OnDetach()
	{
		EB_CORE_INFO("Visibility System detached!");
	}

	void VisibilitySystem::OnSceneDetach(Scene* scene)
	{
		// EntityIDs are per-registry and reused across scenes; drop stale visibility so a newly
		// attached scene's entities don't inherit another scene's cull state.
		m_LastVisibleFrame.clear();
		m_HasCullData = false;
	}

	void VisibilitySystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		EB_PROFILE_FUNCTION();

		// Assume no usable cull until we prove otherwise; while this is false every entity reports
		// visible so consumers never skip work for something that might be on screen.
		m_HasCullData = false;

		if (!m_Enabled)
			return;

		m_FrameIndex++;

		Matrix4f cameraViewProjection;
		if (!TryGetActiveCameraViewProjection(scene, cameraViewProjection))
			return;

		// Build the camera frustum and dilate it outward by a world-space margin. Because this system
		// runs before TransformSystem it culls against last frame's transforms, so the margin (plus the
		// grace window on lookup) keeps the visible set a conservative superset of what actually renders.
		Frustum frustum(cameraViewProjection);
		for (auto& plane : frustum.Planes)
			plane.Distance += m_FrustumMargin;

		GatherRenderableAABBs(scene, m_RenderableScratch);

		for (const auto& [entityID, aabb] : m_RenderableScratch)
		{
			if (!frustum.IsBoxVisible(aabb.WorldMin, aabb.WorldMax))
				continue;

			MarkVisible(entityID);

			// A skinned mesh's animation is driven by a (possibly separate) animator entity — mark it
			// visible too so AnimationSystem keeps evaluating any animator that drives a visible mesh.
			EntityID animatorEntity = ResolveAnimatorEntityID(scene, entityID);
			if (animatorEntity != Constants::Entities::InvalidEntityID)
				MarkVisible(animatorEntity);
		}

		m_HasCullData = true;
	}

	void VisibilitySystem::MarkVisible(EntityID entity)
	{
		m_LastVisibleFrame[entity] = m_FrameIndex;
	}

	bool VisibilitySystem::WasVisibleWithin(EntityID entity, uint32_t graceFrames) const
	{
		// Fail-safe: with no valid cull this frame (system disabled or no active camera) we cannot prove
		// anything is off screen, so treat everything as visible and never skip its simulation.
		if (!m_Enabled || !m_HasCullData)
			return true;

		auto it = m_LastVisibleFrame.find(entity);
		if (it == m_LastVisibleFrame.end())
			return false;

		return (m_FrameIndex - it->second) <= static_cast<uint64_t>(graceFrames);
	}

	void VisibilitySystem::GatherRenderableAABBs(Scene* scene, std::vector<std::pair<EntityID, AABB>>& outEntities)
	{
		auto& registry = scene->GetRegistry();

		const size_t previousSize = outEntities.size();
		outEntities.clear();
		outEntities.reserve(previousSize);

		// NOTE: Render mask filtering is intentionally NOT applied here so that entities excluded by the
		// camera's render mask still cast shadows in the shadow pass. The mask is applied later in
		// RenderSystem::SortEntitiesByRenderQueue when building the visible (non-shadow) render buckets.

		auto transformLocalBoundsToWorldAABB = [](Vector3f localMin, Vector3f localMax, const Matrix4f& worldMat) -> AABB {
			Vector3f corners[8] = {
				{localMin.x, localMin.y, localMin.z},
				{localMax.x, localMin.y, localMin.z},
				{localMin.x, localMax.y, localMin.z},
				{localMax.x, localMax.y, localMin.z},
				{localMin.x, localMin.y, localMax.z},
				{localMax.x, localMin.y, localMax.z},
				{localMin.x, localMax.y, localMax.z},
				{localMax.x, localMax.y, localMax.z}
			};

			Vector3f worldMin = Vector3f(std::numeric_limits<float>::max());
			Vector3f worldMax = Vector3f(std::numeric_limits<float>::lowest());
			for (int i = 0; i < 8; i++)
			{
				Vector3f worldCorner = worldMat * Vector4f(corners[i], 1.0f);
				worldMin = Math::Min(worldMin, worldCorner);
				worldMax = Math::Max(worldMax, worldCorner);
			}
			return AABB{ worldMin, worldMax };
		};

		auto padLocalBounds = [](Vector3f& localMin, Vector3f& localMax, float padding) {
			if (padding == 1.0f)
				return;
			Vector3f center = (localMin + localMax) * 0.5f;
			Vector3f extents = (localMax - localMin) * 0.5f * padding;
			localMin = center - extents;
			localMax = center + extents;
		};

		// Static meshes — cull with the mesh entity's own world transform.
		for (EntityID entityId : registry.ActiveQuery<StaticMeshComponent, MaterialComponent, TransformComponent>())
		{
			Entity entity(entityId, scene);
			auto& meshComp = entity.GetComponent<StaticMeshComponent>();
			if (meshComp.MeshHandle == Constants::InvalidUUID)
				continue;

			auto mesh = Application::Instance().GetAssetManager().GetAsset<Mesh>(meshComp.MeshHandle);
			Vector3f localMin = mesh->GetMinBounds();
			Vector3f localMax = mesh->GetMaxBounds();
			AABB worldAABB = transformLocalBoundsToWorldAABB(
				localMin, localMax, entity.GetComponent<TransformComponent>().GetWorldTransform());
			outEntities.push_back(std::make_pair(entity, worldAABB));
		}

		// Skinned meshes — take the UNION of:
		//   1) mesh-entity world transform x bind-pose bounds
		//   2) animator-root world transform x bind-pose bounds (when available)
		// Mesh bounds are often authored in character/root space while the mesh entity sits under a
		// scaled/rotated armature (e.g. Mixamo). Either matrix alone can be wrong for some hierarchies;
		// the union is conservative (may overdraw, rarely false-culls).
		constexpr float skinnedBoundsPadding = 1.25f;
		for (EntityID entityId : registry.ActiveQuery<SkinnedMeshComponent, MaterialComponent, TransformComponent>())
		{
			Entity entity(entityId, scene);
			auto& meshComp = entity.GetComponent<SkinnedMeshComponent>();
			if (meshComp.MeshHandle == Constants::InvalidUUID)
				continue;

			auto mesh = Application::Instance().GetAssetManager().GetAsset<Mesh>(meshComp.MeshHandle);
			Vector3f localMin = mesh->GetMinBounds();
			Vector3f localMax = mesh->GetMaxBounds();
			padLocalBounds(localMin, localMax, skinnedBoundsPadding);

			const Matrix4f& meshWorldMat = entity.GetComponent<TransformComponent>().GetWorldTransform();
			AABB worldAABB = transformLocalBoundsToWorldAABB(localMin, localMax, meshWorldMat);

			if (meshComp.AnimatorEntityHandle != Constants::InvalidUUID)
			{
				Entity animatorEntity = scene->GetEntity(meshComp.AnimatorEntityHandle);
				if (animatorEntity)
				{
					AABB animatorAABB = transformLocalBoundsToWorldAABB(
						localMin, localMax, animatorEntity.GetComponent<TransformComponent>().GetWorldTransform());
					worldAABB.WorldMin = Math::Min(worldAABB.WorldMin, animatorAABB.WorldMin);
					worldAABB.WorldMax = Math::Max(worldAABB.WorldMax, animatorAABB.WorldMax);
				}
			}

			outEntities.push_back(std::make_pair(entity, worldAABB));
		}
	}

}
