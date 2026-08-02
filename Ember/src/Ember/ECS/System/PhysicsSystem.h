#pragma once

#include "Ember/ECS/Types.h"
#include "System.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Physics/RaycastData.h"
#include "Ember/Physics/CollisionTestCallback.h"
#include "Ember/Physics/OverlapTestCallback.h"
#include "Ember/Physics/PhysicsEventListener.h"
#include "Ember/Physics/VolumeOverlapCallback.h"
#include "Ember/Scene/Entity.h"

// forward declarations
namespace reactphysics3d {
	class PhysicsCommon;
	class PhysicsWorld;
}
namespace rp3d = reactphysics3d;

namespace Ember {

	struct PhysicsSettings
	{
		float GravityStrength = 9.81f;
		Vector3f GravityVector = { 0.0f, -1.0f, 0.0f };

		uint32_t UpdateRate = 60;

		uint32_t PositionSolverIterations = 5;
		uint32_t VelocitySolverIterations = 10;
	};

	struct PhysicsDebugRenderSettings
	{
		bool Enabled = false;

		bool DrawColliders = true;
		bool DrawColliderAxes = true;
		bool DrawContactPoints = true;
	};

	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem();
		virtual ~PhysicsSystem();

		void OnAttach() override;
		void OnDetach() override;
		void OnSceneAttach(Scene* scene) override;
		void OnSceneDetach(Scene* scene) override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnEditorUpdate(TimeStep delta, Scene* scene);

		void RemoveRigidBody(RigidBodyComponent& rigidBody);
		void InitializeEntity(EntityID entity, Scene* scene);
		// Destroys all rp3d bodies/shapes for a prefab hierarchy and clears component physics pointers.
		// Used when runtime prefab instantiation must discard physics created during deserialization hooks.
		void TeardownHierarchyPhysics(EntityID rootEntity, Scene* scene);

		RaycastData CastRay(const Vector3f& startPoint, const Vector3f& direction, float length, Filter filter = FilterPreset::All);
		RaycastData CastRay(const Vector3f& startPoint, const Vector3f& endPoint, Filter filter = FilterPreset::All);

		OverlapTestData TestOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, Filter filter = FilterPreset::All);
		OverlapTestData TestOverlapSphere(const Vector3f& position, float radius, Entity entity, Filter filter = FilterPreset::All);

		CollisionCallbackData TestCollision(Entity entity);

		std::vector<VolumeOverlapData> GetOverlappingVolumes(const Vector3f& cameraPosition, Filter overlapFilter);

		PhysicsSettings& GetSettings() { return m_Settings; }
		void RefreshPhysicsWorld();

		void RestartPhysicsWorld();

		PhysicsDebugRenderSettings& GetDebugRenderSettings() { return m_DebugRenderSettings; }

		// When the selected entity has a collider with PreviewCollider enabled, that collider
		// is drawn in the viewport without needing the global physics debug draw toggle.
		void SetColliderPreviewEntity(EntityID entity) { m_PostProcessDebugEntity = entity; }
		void ClearColliderPreviewEntity() { m_PostProcessDebugEntity = Constants::Entities::InvalidEntityID; }

	private:
		void InitCameraSensor();
		void CreateRigidBody(EntityID entity, TransformComponent& transform, RigidBodyComponent& rigidBody);
		void CreateBoxCollider(EntityID entity, BoxColliderComponent& box, Scene* scene);
		void CreateSphereCollider(EntityID entity, SphereColliderComponent& sphere, Scene* scene);
		void CreateCapsuleCollider(EntityID entity, CapsuleColliderComponent& capsule, Scene* scene);
		void CreateConvexMeshCollider(EntityID entity, ConvexMeshColliderComponent& mesh, Scene* scene);
		void CreateConcaveMeshCollider(EntityID entity, ConcaveMeshColliderComponent& mesh, Scene* scene);
		void UpdateRigidbodies(Scene* scene);
		void SyncChildColliderTransforms(Scene* scene);
		void UpdateAvoidanceCollisions(Scene* scene);
		void UpdateScriptTriggers(Scene* scene);
		void SyncEditorRigidBodies(Scene* scene);
		void RebuildEditorColliders(Scene* scene);
		void DrawSelectedChildColliderPreview(Scene* scene, EntityID selectedEntity);
		bool IsTransformChanged(Scene* scene, EntityID entity, const Matrix4f& cachedWorldTransform) const;
		bool IsScaleUsable(Scene* scene, EntityID entity) const;
		bool HasSupportedColliderComponent(Scene* scene, EntityID entity) const;
		bool ComputeColliderWorldPose(Scene* scene, EntityID selectedEntity, const ColliderOffset& offset, rp3d::Collider* collider,
			Vector3f& outWorldPos, Quaternion& outWorldRot, Vector3f& outChildWorldScale) const;
		void ShowDebugRendererIfApplicable();
		void UpdateDebugRenderData();

	private:
		ScopedPtr<rp3d::PhysicsCommon> m_PhysicsCommon = nullptr;

		rp3d::PhysicsWorld* m_PhysicsWorld = nullptr;	// TODO: Make scoped ptr
		rp3d::RigidBody* m_CameraSensorBody = nullptr;
		rp3d::CollisionShape* m_CameraSensorShape = nullptr;

		PhysicsEventListener m_PhysicsEventListener;

		PhysicsSettings m_Settings;
		PhysicsDebugRenderSettings m_DebugRenderSettings;

		EntityID m_PostProcessDebugEntity = Constants::Entities::InvalidEntityID;

		float m_TimeAcumulator = 0.0f;
	};

}