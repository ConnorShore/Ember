#include "ebpch.h"
#include "PhysicsSystem.h"
#include "ScriptSystem.h"
#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Render/DebugRenderer.h"
#include "Ember/Physics/RaycastCallback.h"
#include "Ember/Physics/ColliderUserData.h"
#include "Ember/Physics/TriggerFilterOverlapCallback.h"
#include "Ember/Physics/VolumeOverlapCallback.h"

#include <reactphysics3d/reactphysics3d.h>

namespace Ember {

	// --- HELPER FUNCTIONS ---

	static rp3d::BodyType ToRp3dBodyType(RigidBodyComponent::BodyType type)
	{
		switch (type)
		{
		case RigidBodyComponent::BodyType::Static:
			return rp3d::BodyType::STATIC;
		case RigidBodyComponent::BodyType::Dynamic:
			return rp3d::BodyType::DYNAMIC;
		case RigidBodyComponent::BodyType::Kinematic:
			return rp3d::BodyType::KINEMATIC;
		default:
			EB_CORE_ASSERT(false, "Unknown body type!");
			return rp3d::BodyType::STATIC;
		}
	}

	// Climbs the Relationship tree to find the EntityID that owns the RigidBody
	static EntityID FindRigidBodyEntity(EntityID current, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		EntityID node = current;

		while (node != Constants::Entities::InvalidEntityID)
		{
			// Found it!
			if (registry.ContainsComponent<RigidBodyComponent>(node))
				return node;

			// Climb up to the parent
			if (registry.ContainsComponent<RelationshipComponent>(node))
			{
				UUID parentUUID = registry.GetComponent<RelationshipComponent>(node).ParentHandle;
				if (parentUUID == Constants::InvalidUUID)
					break; // Reached the root

				Entity parentEntity = scene->GetEntity(parentUUID);
				node = parentEntity.GetEntityHandle();
			}
			else
			{
				break; // No relationship component, cannot climb further
			}
		}
		return Constants::Entities::InvalidEntityID;
	}


	// --- COLLIDER HELPERS ---

	struct ColliderSetupCtx
	{
		RigidBodyComponent* Rb;

		Vector3f RelPos;
		Vector3f RelRot;
		Vector3f ChildWorldScale;
		Matrix4f ChildWorldTransform;
	};

	static bool ResolveColliderSetup(EntityID entity, Scene* scene, ColliderSetupCtx& ctx)
	{
		EntityID rootBodyEntity = FindRigidBodyEntity(entity, scene);
		if (rootBodyEntity == Constants::Entities::InvalidEntityID)
			return false;

		auto& registry = scene->GetRegistry();
		auto& rb = registry.GetComponent<RigidBodyComponent>(rootBodyEntity);
		if (rb.Body == nullptr)
			return false;

		auto& rootTransform = registry.GetComponent<TransformComponent>(rootBodyEntity);
		auto& childTransform = registry.GetComponent<TransformComponent>(entity);

		Matrix4f relativeMatrix = Math::Inverse(rootTransform.WorldTransform) * childTransform.WorldTransform;
		Vector3f relScale;
		Math::DecomposeTransform(relativeMatrix, ctx.RelPos, ctx.RelRot, relScale);

		Vector3f childWorldPos, childWorldRot;
		Math::DecomposeTransform(childTransform.WorldTransform, childWorldPos, childWorldRot, ctx.ChildWorldScale);
		ctx.ChildWorldTransform = childTransform.WorldTransform;
		// Override with column-length extraction for a numerically stable scale that matches ScaleChanged
		ctx.ChildWorldScale = Vector3f(
			glm::length(Vector3f(childTransform.WorldTransform[0])),
			glm::length(Vector3f(childTransform.WorldTransform[1])),
			glm::length(Vector3f(childTransform.WorldTransform[2]))
		);

		ctx.Rb = &rb;
		return true;
	}

	static rp3d::Transform MakeColliderTransform(const Vector3f& relPos, const Vector3f& relRot, const ColliderOffset& offset = ColliderOffset())
	{
		Quaternion entityQuat = Math::ToQuaternion(relRot);
		Quaternion offsetQuat = Math::ToQuaternion(offset.Rotation);
		Quaternion finalQuat = entityQuat * offsetQuat;
		return rp3d::Transform(
			rp3d::Vector3(relPos.x + offset.Position.x, relPos.y + offset.Position.y, relPos.z + offset.Position.z),
			rp3d::Quaternion(finalQuat.x, finalQuat.y, finalQuat.z, finalQuat.w)
		);
	}

	template<typename TCollider, typename TShape>
	static void AttachAndUpdateMass(EntityID entity, TCollider& collider, TShape* shape, RigidBodyComponent& rb, const rp3d::Transform& localTransform)
	{
		collider.Shape = shape;
		collider.AttachedBody = rb.Body;

		// Setup collider in the physics engine
		collider.Collider = rb.Body->addCollider(shape, localTransform);

		// Store the entity ID in the user data of the collider for easy retrieval during raycasts and collision events
		collider.UserData = { entity, collider.Category };
		// Store a stable raw EntityID value in RP3D user data. Pointers to component-owned
		// structs can become stale when sparse-set storage relocates components on spawn/despawn.
		collider.Collider->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

		// Set trigger
		collider.Collider->setIsTrigger(collider.IsTrigger);

		// Set collision filters
		if (collider.Category != FilterPreset::Default)
			collider.Collider->setCollisionCategoryBits(collider.Category);

		if (collider.CollisionMask != FilterPreset::Default)
			collider.Collider->setCollideWithMaskBits(collider.CollisionMask);

		if (collider.PhysicsMaterialHandle != Constants::InvalidUUID)
		{
			auto material = Application::Instance().GetAssetManager().GetAsset<PhysicsMaterial>(collider.PhysicsMaterialHandle);
			EB_CORE_ASSERT(material, "Invalid Physics Material handle!");

			auto& rp3dMaterial = collider.Collider->getMaterial();
			rp3dMaterial.setBounciness(material->Bounciness);
			rp3dMaterial.setFrictionCoefficient(material->Friction);
		}

		if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
		{
			rb.Body->updateMassPropertiesFromColliders();
			if (rb.Mass > 0.0f)
			{
				float currentMass = rb.Body->getMass();
				if (currentMass > 0.0f)
				{
					float massRatio = rb.Mass / currentMass;
					rp3d::Vector3 localInertia = rb.Body->getLocalInertiaTensor();
					rb.Body->setMass(rb.Mass);
					rb.Body->setLocalInertiaTensor(localInertia * massRatio);
				}
			}
		}
	}

	template<typename TCollider, typename TDestroyShape>
	static void DetachCollider(TCollider& collider, TDestroyShape&& destroyShape)
	{
		if (collider.Collider && collider.AttachedBody)
		{
			collider.AttachedBody->removeCollider(collider.Collider);
			if (collider.Shape)
				destroyShape();
			collider.Collider = nullptr;
			collider.Shape = nullptr;
			collider.AttachedBody = nullptr;
		}
	}

	// --- PHYSICS SYSTEM IMPLEMENTATION ---

	PhysicsSystem::PhysicsSystem()
	{
	}

	PhysicsSystem::~PhysicsSystem()
	{
		if (m_PhysicsWorld)
		{
			m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);
			m_PhysicsWorld = nullptr;
		}
	}

	void PhysicsSystem::OnAttach()
	{
		m_PhysicsCommon = ScopedPtr<rp3d::PhysicsCommon>::Create();

		m_PhysicsWorld = m_PhysicsCommon->createPhysicsWorld();
		RefreshPhysicsWorld();

		m_PhysicsWorld->setEventListener(&m_PhysicsEventListener);

		InitCameraSensor();

		EB_CORE_INFO("Physics System attached!");
	}

	void PhysicsSystem::OnDetach()
	{
		if (m_PhysicsWorld)
		{
			m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);
			m_PhysicsWorld = nullptr;
		}

		EB_CORE_INFO("Physics System detached!");
	}

	void PhysicsSystem::OnSceneAttach(Scene* scene)
	{
		// Reset all stale physics pointers on the scene's entities before destroying the old
		// RP3D world. Without this, ConnectAndRetroact skips body creation for any entity
		// whose rb.Body is non-null (i.e. pointing into the about-to-be-destroyed world).
		scene->ResetAllPhysicsState();

		RestartPhysicsWorld();

		auto& registry = scene->GetRegistry();

		// Setup debug renderer
		ShowDebugRendererIfApplicable();

		// Creation hooks
		registry.ConnectAndRetroact<RigidBodyComponent>(
			[this, scene](EntityID entity, RigidBodyComponent& rb) {
				if (rb.Body == nullptr)
				{
					auto& transform = scene->GetRegistry().GetComponent<TransformComponent>(entity);
					this->CreateRigidBody(entity, transform, rb);
				}
			}
		);

		registry.ConnectAndRetroact<BoxColliderComponent>(
			[this, scene](EntityID entity, BoxColliderComponent& box) {
				if (box.Shape != nullptr)
					return;
				CreateBoxCollider(entity, box, scene);
			}
		);

		registry.ConnectAndRetroact<SphereColliderComponent>(
			[this, scene](EntityID entity, SphereColliderComponent& sphere) {
				if (sphere.Shape != nullptr)
					return;
				CreateSphereCollider(entity, sphere, scene);
			}
		);

		registry.ConnectAndRetroact<ConvexMeshColliderComponent>(
			[this, scene](EntityID entity, ConvexMeshColliderComponent& mesh) {
				if (mesh.Shape != nullptr)
					return;
				CreateConvexMeshCollider(entity, mesh, scene);
			}
		);

		registry.ConnectAndRetroact<ConcaveMeshColliderComponent>(
			[this, scene](EntityID entity, ConcaveMeshColliderComponent& mesh) {
				if (mesh.Shape != nullptr)
					return;
				CreateConcaveMeshCollider(entity, mesh, scene);
			}
		);

		registry.ConnectAndRetroact<CapsuleColliderComponent>(
			[this, scene](EntityID entity, CapsuleColliderComponent& capsule) {
				if (capsule.Shape != nullptr)
					return;
				CreateCapsuleCollider(entity, capsule, scene);
			}
		);

		// Cleanup hooks
		registry.OnComponentDetached<RigidBodyComponent>().Connect(
			[this, scene](EntityID entity, RigidBodyComponent& rb) {
				if (rb.Body) {
					Entity e(entity, scene);
					// detatch all rigid bodies from collider component types
					for (uint32_t i = 0; i < rb.Body->getNbColliders(); i++)
					{
						// Helper function to remove body reference from a collider component if it matches the current body
						auto removeColliderBodyFunction = [&](auto& colliderComponent) {
							if (colliderComponent.AttachedBody == rb.Body)
							{
								colliderComponent.AttachedBody = nullptr;
								colliderComponent.Collider = nullptr;
							}
						};

						// Remove body from collider component based on the collision shape type
						auto collisionShape = rb.Body->getCollider(i)->getCollisionShape();
						switch (collisionShape->getName())
						{
						case reactphysics3d::CollisionShapeName::BOX:
							removeColliderBodyFunction(e.GetComponent<BoxColliderComponent>());
							break;
						case reactphysics3d::CollisionShapeName::SPHERE:
							removeColliderBodyFunction(e.GetComponent<SphereColliderComponent>());
							break;
						case reactphysics3d::CollisionShapeName::CAPSULE:
							removeColliderBodyFunction(e.GetComponent<CapsuleColliderComponent>());
							break;
						case reactphysics3d::CollisionShapeName::CONVEX_MESH:
							removeColliderBodyFunction(e.GetComponent<ConvexMeshColliderComponent>());
							break;
						case reactphysics3d::CollisionShapeName::TRIANGLE_MESH:
							removeColliderBodyFunction(e.GetComponent<ConcaveMeshColliderComponent>());
							break;
						default:
							EB_CORE_ASSERT(false, "Unknown collider type attached to rigid body! Cannot clean up properly.");
							break;
						}
					}

					m_PhysicsWorld->destroyRigidBody(rb.Body);
					rb.Body = nullptr;
				}
			}
		);

		registry.OnComponentDetached<BoxColliderComponent>().Connect(
			[this](EntityID entity, BoxColliderComponent& box) {
				DetachCollider(box, [&]() { m_PhysicsCommon->destroyBoxShape(box.Shape); });
			}
		);

		registry.OnComponentDetached<SphereColliderComponent>().Connect(
			[this](EntityID entity, SphereColliderComponent& sphere) {
				DetachCollider(sphere, [&]() { m_PhysicsCommon->destroySphereShape(sphere.Shape); });
			}
		);

		registry.OnComponentDetached<CapsuleColliderComponent>().Connect(
			[this](EntityID entity, CapsuleColliderComponent& capsule) {
				DetachCollider(capsule, [&]() { m_PhysicsCommon->destroyCapsuleShape(capsule.Shape); });
			}
		);

		registry.OnComponentDetached<ConvexMeshColliderComponent>().Connect(
			[this](EntityID entity, ConvexMeshColliderComponent& mesh) {
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConvexMeshShape(mesh.Shape);
					if (mesh.RP3DVertexArray)
						delete mesh.RP3DVertexArray;
					mesh.RP3DVertexArray = nullptr;
				});
			}
		);

		registry.OnComponentDetached<ConcaveMeshColliderComponent>().Connect(
			[this](EntityID entity, ConcaveMeshColliderComponent& mesh) {
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConcaveMeshShape(mesh.Shape);
					if (mesh.TriangleMesh)
						m_PhysicsCommon->destroyTriangleMesh(mesh.TriangleMesh);
					if (mesh.TriangleArray)
						delete mesh.TriangleArray;
					mesh.TriangleMesh = nullptr;
					mesh.TriangleArray = nullptr;
				});
			}
		);

		// Disabled component attach/detach.  Need to enable/disable rigid body for this component
		registry.ConnectAndRetroact<DisabledComponent>(
			[this, scene](EntityID entity, DisabledComponent& dc) {
				auto& registry = scene->GetRegistry();
				if (!registry.ContainsComponent<RigidBodyComponent>(entity))
					return;
				auto& rbComp = registry.GetComponent<RigidBodyComponent>(entity);
				if (rbComp.Body)
					rbComp.Body->setIsActive(false);
			}
		);

		registry.OnComponentDetached<DisabledComponent>().Connect(
			[this, scene](EntityID entity, DisabledComponent& dc) {
				auto& registry = scene->GetRegistry();
				if (!registry.ContainsComponent<RigidBodyComponent>(entity))
					return;
				auto& rbComp = registry.GetComponent<RigidBodyComponent>(entity);
				if (rbComp.Body)
					rbComp.Body->setIsActive(true);
			}
		);
	}

	void PhysicsSystem::OnSceneDetach(Scene* scene)
	{

	}

	void PhysicsSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		EB_PROFILE_FUNCTION();

		ShowDebugRendererIfApplicable();

		const float timeStep = 1.0f / m_Settings.UpdateRate;
		m_TimeAcumulator += delta;

		// Step the physics simulation
		{
			EB_PROFILE_SCOPE("PhysicsSystem::Simulate");
			while (m_TimeAcumulator >= timeStep)
			{
				m_PhysicsWorld->update(timeStep);
				m_TimeAcumulator -= timeStep;
			}
		}

		// Update rigid body transforms from the physics simulation results
		{
			EB_PROFILE_SCOPE("PhysicsSystem::UpdateRigidbodies");
			UpdateRigidbodies(scene);
		}

		// Update script triggers
		{
			EB_PROFILE_SCOPE("PhysicsSystem::UpdateScriptTriggers");
			UpdateScriptTriggers(scene);
		}

		// Update Local Avoidance vectors
		{
			EB_PROFILE_SCOPE("PhysicsSystem::UpdateAvoidanceCollisions");
			UpdateAvoidanceCollisions(scene);
		}

		// Update debug render data
		{
			EB_PROFILE_SCOPE("PhysicsSystem::UpdateDebugRenderData");
			UpdateDebugRenderData();
		}
	}

	void PhysicsSystem::OnEditorUpdate(TimeStep delta, Scene* scene)
	{
		EB_PROFILE_FUNCTION();

		{
			EB_PROFILE_SCOPE("PhysicsSystem::SyncEditorRigidBodies");
			SyncEditorRigidBodies(scene);
		}
		{
			EB_PROFILE_SCOPE("PhysicsSystem::RebuildEditorColliders");
			RebuildEditorColliders(scene);
		}

		if (m_PostProcessDebugEntity != Constants::Entities::InvalidEntityID)
			DrawSelectedChildColliderPreview(scene, m_PostProcessDebugEntity);

		ShowDebugRendererIfApplicable();
		UpdateDebugRenderData();
	}

	void PhysicsSystem::SyncEditorRigidBodies(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<RigidBodyComponent, TransformComponent>();

		for (EntityID entity : view)
		{
			auto [rb, transform] = registry.GetComponents<RigidBodyComponent, TransformComponent>(entity);

			if (rb.Body == nullptr)
				continue;

			bool isPostProcessDebugEntity = (m_PostProcessDebugEntity != Constants::Entities::InvalidEntityID && entity == m_PostProcessDebugEntity);
			rb.Body->setIsDebugEnabled(m_DebugRenderSettings.Enabled || isPostProcessDebugEntity);

			rb.Body->setType(ToRp3dBodyType(rb.Type));
			rb.Body->enableGravity(rb.GravityEnabled);

			Vector3f worldPos, worldRot, worldScale;
			Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

			rp3d::Vector3 newPos(worldPos.x, worldPos.y, worldPos.z);
			Quaternion q = Math::ToQuaternion(worldRot);
			rp3d::Quaternion newRot(q.x, q.y, q.z, q.w);

			rb.Body->setTransform(rp3d::Transform(newPos, newRot));
		}
	}

	bool PhysicsSystem::IsTransformChanged(Scene* scene, EntityID entity, const Matrix4f& cachedWorldTransform) const
	{
		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<TransformComponent>(entity))
			return false;

		const auto& t = registry.GetComponent<TransformComponent>(entity);
		const float epsilon = 1e-5f;
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				if (glm::abs(t.WorldTransform[column][row] - cachedWorldTransform[column][row]) > epsilon)
					return true;
			}
		}

		return false;
	}

	bool PhysicsSystem::IsScaleUsable(Scene* scene, EntityID entity) const
	{
		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<TransformComponent>(entity))
			return false;

		const auto& t = registry.GetComponent<TransformComponent>(entity);
		const float minScale = 1e-4f;
		return glm::length(Vector3f(t.WorldTransform[0])) > minScale &&
			glm::length(Vector3f(t.WorldTransform[1])) > minScale &&
			glm::length(Vector3f(t.WorldTransform[2])) > minScale;
	}

	void PhysicsSystem::RebuildEditorColliders(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		auto boxView = registry.ActiveQuery<BoxColliderComponent>();
		for (EntityID entity : boxView)
		{
			auto& box = registry.GetComponent<BoxColliderComponent>(entity);
			if ((box.NeedsRebuild || IsTransformChanged(scene, entity, box.CachedWorldTransform)) && IsScaleUsable(scene, entity))
			{
				DetachCollider(box, [&]() { m_PhysicsCommon->destroyBoxShape(box.Shape); });
				CreateBoxCollider(entity, box, scene);
				box.NeedsRebuild = false;
			}
		}

		auto sphereView = registry.ActiveQuery<SphereColliderComponent>();
		for (EntityID entity : sphereView)
		{
			auto& sphere = registry.GetComponent<SphereColliderComponent>(entity);
			if ((sphere.NeedsRebuild || IsTransformChanged(scene, entity, sphere.CachedWorldTransform)) && IsScaleUsable(scene, entity))
			{
				DetachCollider(sphere, [&]() { m_PhysicsCommon->destroySphereShape(sphere.Shape); });
				CreateSphereCollider(entity, sphere, scene);
				sphere.NeedsRebuild = false;
			}
		}

		auto capsuleView = registry.ActiveQuery<CapsuleColliderComponent>();
		for (EntityID entity : capsuleView)
		{
			auto& capsule = registry.GetComponent<CapsuleColliderComponent>(entity);
			if ((capsule.NeedsRebuild || IsTransformChanged(scene, entity, capsule.CachedWorldTransform)) && IsScaleUsable(scene, entity))
			{
				DetachCollider(capsule, [&]() { m_PhysicsCommon->destroyCapsuleShape(capsule.Shape); });
				CreateCapsuleCollider(entity, capsule, scene);
				capsule.NeedsRebuild = false;
			}
		}

		auto convexView = registry.ActiveQuery<ConvexMeshColliderComponent>();
		for (EntityID entity : convexView)
		{
			auto& mesh = registry.GetComponent<ConvexMeshColliderComponent>(entity);
			if ((mesh.NeedsRebuild || IsTransformChanged(scene, entity, mesh.CachedWorldTransform)) && IsScaleUsable(scene, entity))
			{
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConvexMeshShape(mesh.Shape);
					if (mesh.RP3DVertexArray)
						delete mesh.RP3DVertexArray;
					mesh.RP3DVertexArray = nullptr;
				});
				CreateConvexMeshCollider(entity, mesh, scene);
				mesh.NeedsRebuild = false;
			}
		}

		auto concaveView = registry.ActiveQuery<ConcaveMeshColliderComponent>();
		for (EntityID entity : concaveView)
		{
			auto& mesh = registry.GetComponent<ConcaveMeshColliderComponent>(entity);
			if ((mesh.NeedsRebuild || IsTransformChanged(scene, entity, mesh.CachedWorldTransform)) && IsScaleUsable(scene, entity))
			{
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConcaveMeshShape(mesh.Shape);
					if (mesh.TriangleMesh)
						m_PhysicsCommon->destroyTriangleMesh(mesh.TriangleMesh);
					if (mesh.TriangleArray)
						delete mesh.TriangleArray;
					mesh.TriangleMesh = nullptr;
					mesh.TriangleArray = nullptr;
				});
				CreateConcaveMeshCollider(entity, mesh, scene);
				mesh.NeedsRebuild = false;
			}
		}
	}

	bool PhysicsSystem::HasSupportedColliderComponent(Scene* scene, EntityID entity) const
	{
		auto& registry = scene->GetRegistry();
		return registry.ContainsComponent<BoxColliderComponent>(entity) ||
			registry.ContainsComponent<SphereColliderComponent>(entity) ||
			registry.ContainsComponent<CapsuleColliderComponent>(entity) ||
			registry.ContainsComponent<ConvexMeshColliderComponent>(entity) ||
			registry.ContainsComponent<ConcaveMeshColliderComponent>(entity);
	}

	bool PhysicsSystem::ComputeColliderWorldPose(Scene* scene, EntityID selectedEntity, const ColliderOffset& offset, rp3d::Collider* collider,
		Vector3f& outWorldPos, Quaternion& outWorldRot, Vector3f& outChildWorldScale) const
	{
		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(selectedEntity, scene, ctx))
			return false;

		EntityID rootBodyEntity = FindRigidBodyEntity(selectedEntity, scene);
		if (rootBodyEntity == Constants::Entities::InvalidEntityID)
			return false;

		if (collider != nullptr)
		{
			const rp3d::Transform colliderWorldTransform = collider->getLocalToWorldTransform();
			const rp3d::Vector3 worldPos = colliderWorldTransform.getPosition();
			const rp3d::Quaternion worldOrientation = colliderWorldTransform.getOrientation();

			outWorldPos = { worldPos.x, worldPos.y, worldPos.z };
			outWorldRot = Math::Normalize(Quaternion(worldOrientation.w, worldOrientation.x, worldOrientation.y, worldOrientation.z));
		}
		else
		{
			rp3d::Transform localColliderTransform = MakeColliderTransform(ctx.RelPos, ctx.RelRot, offset);
			const rp3d::Transform& rootBodyTransform = ctx.Rb->Body->getTransform();

			const rp3d::Vector3 rootPos = rootBodyTransform.getPosition();
			const rp3d::Quaternion rootOrientation = rootBodyTransform.getOrientation();
			const rp3d::Vector3 localPos = localColliderTransform.getPosition();
			const rp3d::Quaternion localOrientation = localColliderTransform.getOrientation();

			const Quaternion rootQuat(rootOrientation.w, rootOrientation.x, rootOrientation.y, rootOrientation.z);
			const Quaternion localQuat(localOrientation.w, localOrientation.x, localOrientation.y, localOrientation.z);
			outWorldRot = Math::Normalize(rootQuat * localQuat);

			const Vector3f localOffset = { localPos.x, localPos.y, localPos.z };
			outWorldPos = { rootPos.x, rootPos.y, rootPos.z } + Math::Rotate(rootQuat, localOffset);
		}

		outChildWorldScale = ctx.ChildWorldScale;
		return true;
	}

	void PhysicsSystem::DrawSelectedChildColliderPreview(Scene* scene, EntityID selectedEntity)
	{
		auto& registry = scene->GetRegistry();
		if (registry.ContainsComponent<RigidBodyComponent>(selectedEntity) || !HasSupportedColliderComponent(scene, selectedEntity))
			return;

		const Vector4f previewColor = { 0.15f, 0.95f, 0.35f, 1.0f };

		auto DrawWireBox = [&](const Vector3f& center, const Quaternion& rotation, const Vector3f& halfExtents, const Vector4f& color)
			{
				Vector3f corners[8] = {
					{-halfExtents.x, -halfExtents.y, -halfExtents.z},
					{ halfExtents.x, -halfExtents.y, -halfExtents.z},
					{ halfExtents.x,  halfExtents.y, -halfExtents.z},
					{-halfExtents.x,  halfExtents.y, -halfExtents.z},
					{-halfExtents.x, -halfExtents.y,  halfExtents.z},
					{ halfExtents.x, -halfExtents.y,  halfExtents.z},
					{ halfExtents.x,  halfExtents.y,  halfExtents.z},
					{-halfExtents.x,  halfExtents.y,  halfExtents.z}
				};

				for (uint32_t i = 0; i < 8; i++)
					corners[i] = center + Math::Rotate(rotation, corners[i]);

				const uint32_t edges[12][2] = {
					{0, 1}, {1, 2}, {2, 3}, {3, 0},
					{4, 5}, {5, 6}, {6, 7}, {7, 4},
					{0, 4}, {1, 5}, {2, 6}, {3, 7}
				};

				for (const auto& edge : edges)
					DebugRenderer::DrawLine(corners[edge[0]], corners[edge[1]], color);
			};

		auto DrawWireCircle = [&](const Vector3f& center, const Vector3f& axisA, const Vector3f& axisB, float radius, uint32_t segments, const Vector4f& color)
			{
				const float twoPi = 6.28318530718f;
				Vector3f prev = center + axisA * radius;
				for (uint32_t i = 1; i <= segments; i++)
				{
					float t = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;
					Vector3f next = center + (axisA * std::cos(t) + axisB * std::sin(t)) * radius;
					DebugRenderer::DrawLine(prev, next, color);
					prev = next;
				}
			};

		auto DrawWireSphere = [&](const Vector3f& center, const Quaternion& rotation, float radius, const Vector4f& color)
			{
				const uint32_t circleSegments = 32;
				const Vector3f right = Math::Rotate(rotation, Vector3f(1.0f, 0.0f, 0.0f));
				const Vector3f up = Math::Rotate(rotation, Vector3f(0.0f, 1.0f, 0.0f));
				const Vector3f forward = Math::Rotate(rotation, Vector3f(0.0f, 0.0f, 1.0f));

				DrawWireCircle(center, right, up, radius, circleSegments, color);
				DrawWireCircle(center, right, forward, radius, circleSegments, color);
				DrawWireCircle(center, up, forward, radius, circleSegments, color);
			};

		auto DrawWireCapsule = [&](const Vector3f& center, const Quaternion& rotation, float radius, float cylinderHeight, const Vector4f& color)
			{
				const uint32_t circleSegments = 32;
				const Vector3f right = Math::Rotate(rotation, Vector3f(1.0f, 0.0f, 0.0f));
				const Vector3f up = Math::Rotate(rotation, Vector3f(0.0f, 1.0f, 0.0f));
				const Vector3f forward = Math::Rotate(rotation, Vector3f(0.0f, 0.0f, 1.0f));

				const float halfCylinder = cylinderHeight * 0.5f;
				const Vector3f topCenter = center + up * halfCylinder;
				const Vector3f bottomCenter = center - up * halfCylinder;

				DrawWireCircle(topCenter, right, forward, radius, circleSegments, color);
				DrawWireCircle(bottomCenter, right, forward, radius, circleSegments, color);

				DebugRenderer::DrawLine(topCenter + right * radius, bottomCenter + right * radius, color);
				DebugRenderer::DrawLine(topCenter - right * radius, bottomCenter - right * radius, color);
				DebugRenderer::DrawLine(topCenter + forward * radius, bottomCenter + forward * radius, color);
				DebugRenderer::DrawLine(topCenter - forward * radius, bottomCenter - forward * radius, color);
			};

		if (registry.ContainsComponent<BoxColliderComponent>(selectedEntity))
		{
			auto& box = registry.GetComponent<BoxColliderComponent>(selectedEntity);
			Vector3f worldPos, childScale;
			Quaternion worldRot;
			if (ComputeColliderWorldPose(scene, selectedEntity, box.Offset, box.Collider, worldPos, worldRot, childScale))
			{
				Vector3f halfExtents(
					(box.Size.x * childScale.x) * 0.5f,
					(box.Size.y * childScale.y) * 0.5f,
					(box.Size.z * childScale.z) * 0.5f
				);
				halfExtents = Math::Max(halfExtents, Vector3f(0.001f));
				DrawWireBox(worldPos, worldRot, halfExtents, previewColor);
			}
		}

		if (registry.ContainsComponent<SphereColliderComponent>(selectedEntity))
		{
			auto& sphere = registry.GetComponent<SphereColliderComponent>(selectedEntity);
			Vector3f worldPos, childScale;
			Quaternion worldRot;
			if (ComputeColliderWorldPose(scene, selectedEntity, sphere.Offset, sphere.Collider, worldPos, worldRot, childScale))
			{
				float radius = sphere.Radius * std::max({ childScale.x, childScale.y, childScale.z });
				radius = std::max(radius, 0.001f);
				DrawWireSphere(worldPos, worldRot, radius, previewColor);
			}
		}

		if (registry.ContainsComponent<CapsuleColliderComponent>(selectedEntity))
		{
			auto& capsule = registry.GetComponent<CapsuleColliderComponent>(selectedEntity);
			Vector3f worldPos, childScale;
			Quaternion worldRot;
			if (ComputeColliderWorldPose(scene, selectedEntity, capsule.Offset, capsule.Collider, worldPos, worldRot, childScale))
			{
				float maxScale = std::max({ childScale.x, childScale.y, childScale.z });
				float radius = std::max(capsule.Radius * maxScale, 0.001f);
				float height = std::max(capsule.Height * maxScale, 0.001f);
				float cylinderHeight = std::max(0.0f, height - 2.0f * radius);
				DrawWireCapsule(worldPos, worldRot, radius, cylinderHeight, previewColor);
				const Vector3f up = Math::Rotate(worldRot, Vector3f(0.0f, 1.0f, 0.0f));
				const float halfCylinder = cylinderHeight * 0.5f;
				DrawWireSphere(worldPos + up * halfCylinder, worldRot, radius, previewColor);
				DrawWireSphere(worldPos - up * halfCylinder, worldRot, radius, previewColor);
			}
		}

		auto DrawMeshBoundsPreview = [&](UUID meshHandle, const ColliderOffset& offset, rp3d::Collider* collider)
			{
				if (meshHandle == Constants::InvalidUUID)
					return;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(meshHandle);
				if (!meshAsset)
					return;

				Vector3f worldPos, childScale;
				Quaternion worldRot;
				if (!ComputeColliderWorldPose(scene, selectedEntity, offset, collider, worldPos, worldRot, childScale))
					return;

				const Vector3f localMin = meshAsset->GetMinBounds();
				const Vector3f localMax = meshAsset->GetMaxBounds();

				Vector3f scaledMin(localMin.x * childScale.x, localMin.y * childScale.y, localMin.z * childScale.z);
				Vector3f scaledMax(localMax.x * childScale.x, localMax.y * childScale.y, localMax.z * childScale.z);

				Vector3f corners[8] = {
					{scaledMin.x, scaledMin.y, scaledMin.z},
					{scaledMax.x, scaledMin.y, scaledMin.z},
					{scaledMax.x, scaledMax.y, scaledMin.z},
					{scaledMin.x, scaledMax.y, scaledMin.z},
					{scaledMin.x, scaledMin.y, scaledMax.z},
					{scaledMax.x, scaledMin.y, scaledMax.z},
					{scaledMax.x, scaledMax.y, scaledMax.z},
					{scaledMin.x, scaledMax.y, scaledMax.z}
				};

				for (uint32_t i = 0; i < 8; i++)
					corners[i] = worldPos + Math::Rotate(worldRot, corners[i]);

				const uint32_t edges[12][2] = {
					{0, 1}, {1, 2}, {2, 3}, {3, 0},
					{4, 5}, {5, 6}, {6, 7}, {7, 4},
					{0, 4}, {1, 5}, {2, 6}, {3, 7}
				};

				for (const auto& edge : edges)
					DebugRenderer::DrawLine(corners[edge[0]], corners[edge[1]], previewColor);
			};

		if (registry.ContainsComponent<ConvexMeshColliderComponent>(selectedEntity))
		{
			auto& convex = registry.GetComponent<ConvexMeshColliderComponent>(selectedEntity);
			DrawMeshBoundsPreview(convex.MeshHandle, convex.Offset, convex.Collider);
		}

		if (registry.ContainsComponent<ConcaveMeshColliderComponent>(selectedEntity))
		{
			auto& concave = registry.GetComponent<ConcaveMeshColliderComponent>(selectedEntity);
			DrawMeshBoundsPreview(concave.MeshHandle, concave.Offset, concave.Collider);
		}
	}

	void PhysicsSystem::RemoveRigidBody(RigidBodyComponent& rigidBody)
	{
		m_PhysicsWorld->destroyRigidBody(rigidBody.Body);
		rigidBody.Body = nullptr;
	}

	void PhysicsSystem::TeardownHierarchyPhysics(EntityID rootEntity, Scene* scene)
	{
		if (!m_PhysicsWorld || rootEntity == Constants::Entities::InvalidEntityID)
			return;

		auto& registry = scene->GetRegistry();

		std::function<void(EntityID)> teardownEntity = [&](EntityID entity) {
			if (entity == Constants::Entities::InvalidEntityID)
				return;

			if (registry.ContainsComponent<RelationshipComponent>(entity))
			{
				for (UUID childUUID : registry.GetComponent<RelationshipComponent>(entity).Children)
				{
					Entity child = scene->GetEntity(childUUID);
					if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
						teardownEntity(child.GetEntityHandle());
				}
			}

			if (registry.ContainsComponent<BoxColliderComponent>(entity))
			{
				auto& box = registry.GetComponent<BoxColliderComponent>(entity);
				DetachCollider(box, [&]() { m_PhysicsCommon->destroyBoxShape(box.Shape); });
				box.NeedsRebuild = false;
			}

			if (registry.ContainsComponent<SphereColliderComponent>(entity))
			{
				auto& sphere = registry.GetComponent<SphereColliderComponent>(entity);
				DetachCollider(sphere, [&]() { m_PhysicsCommon->destroySphereShape(sphere.Shape); });
				sphere.NeedsRebuild = false;
			}

			if (registry.ContainsComponent<CapsuleColliderComponent>(entity))
			{
				auto& capsule = registry.GetComponent<CapsuleColliderComponent>(entity);
				DetachCollider(capsule, [&]() { m_PhysicsCommon->destroyCapsuleShape(capsule.Shape); });
				capsule.NeedsRebuild = false;
			}

			if (registry.ContainsComponent<ConvexMeshColliderComponent>(entity))
			{
				auto& mesh = registry.GetComponent<ConvexMeshColliderComponent>(entity);
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConvexMeshShape(mesh.Shape);
					if (mesh.RP3DVertexArray)
						delete mesh.RP3DVertexArray;
					mesh.RP3DVertexArray = nullptr;
				});
				mesh.NeedsRebuild = false;
			}

			if (registry.ContainsComponent<ConcaveMeshColliderComponent>(entity))
			{
				auto& mesh = registry.GetComponent<ConcaveMeshColliderComponent>(entity);
				DetachCollider(mesh, [&]() {
					m_PhysicsCommon->destroyConcaveMeshShape(mesh.Shape);
					if (mesh.TriangleMesh)
						m_PhysicsCommon->destroyTriangleMesh(mesh.TriangleMesh);
					if (mesh.TriangleArray)
						delete mesh.TriangleArray;
					mesh.TriangleMesh = nullptr;
					mesh.TriangleArray = nullptr;
				});
				mesh.NeedsRebuild = false;
			}

			if (registry.ContainsComponent<RigidBodyComponent>(entity))
			{
				auto& rb = registry.GetComponent<RigidBodyComponent>(entity);
				if (rb.Body)
					RemoveRigidBody(rb);
			}
		};

		teardownEntity(rootEntity);
	}

	void PhysicsSystem::InitializeEntity(EntityID entity, Scene* scene)
	{
		if (!m_PhysicsWorld)
			return;

		auto& registry = scene->GetRegistry();

		if (registry.ContainsComponent<RigidBodyComponent>(entity))
		{
			auto& rb = registry.GetComponent<RigidBodyComponent>(entity);
			if (rb.Body == nullptr)
			{
				auto& transform = registry.GetComponent<TransformComponent>(entity);
				CreateRigidBody(entity, transform, rb);
			}
		}

		if (registry.ContainsComponent<BoxColliderComponent>(entity))
		{
			auto& box = registry.GetComponent<BoxColliderComponent>(entity);
			if (box.Shape == nullptr)
				CreateBoxCollider(entity, box, scene);
		}

		if (registry.ContainsComponent<SphereColliderComponent>(entity))
		{
			auto& sphere = registry.GetComponent<SphereColliderComponent>(entity);
			if (sphere.Shape == nullptr)
				CreateSphereCollider(entity, sphere, scene);
		}

		if (registry.ContainsComponent<CapsuleColliderComponent>(entity))
		{
			auto& capsule = registry.GetComponent<CapsuleColliderComponent>(entity);
			if (capsule.Shape == nullptr)
				CreateCapsuleCollider(entity, capsule, scene);
		}

		if (registry.ContainsComponent<ConvexMeshColliderComponent>(entity))
		{
			auto& mesh = registry.GetComponent<ConvexMeshColliderComponent>(entity);
			if (mesh.Shape == nullptr)
				CreateConvexMeshCollider(entity, mesh, scene);
		}

		if (registry.ContainsComponent<ConcaveMeshColliderComponent>(entity))
		{
			auto& mesh = registry.GetComponent<ConcaveMeshColliderComponent>(entity);
			if (mesh.Shape == nullptr)
				CreateConcaveMeshCollider(entity, mesh, scene);
		}
	}

	void PhysicsSystem::RefreshPhysicsWorld()
	{
		// Set gravity vector (normalized direction * gravity strength)
		rp3d::Vector3 gravityVec(m_Settings.GravityVector.x , m_Settings.GravityVector.y, m_Settings.GravityVector.z);
		gravityVec.normalize();
		gravityVec *= m_Settings.GravityStrength;

		m_PhysicsWorld->setGravity(gravityVec);
		m_PhysicsWorld->setNbIterationsPositionSolver(m_Settings.PositionSolverIterations);
		m_PhysicsWorld->setNbIterationsVelocitySolver(m_Settings.VelocitySolverIterations);
	}

	void PhysicsSystem::RestartPhysicsWorld()
	{
		if (m_PhysicsWorld) {
			// Destroy the world first — this cleans up all bodies and colliders (including
			// the camera sensor body) before we release the shape they reference.
			m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);

			// Now it's safe to release the shape owned by PhysicsCommon.
			if (m_CameraSensorShape)
			{
				m_PhysicsCommon->destroySphereShape(static_cast<rp3d::SphereShape*>(m_CameraSensorShape));
				m_CameraSensorShape = nullptr;
			}

			m_CameraSensorBody = nullptr;
		}
		m_PhysicsWorld = m_PhysicsCommon->createPhysicsWorld();

		// Re-apply the listener and settings to the NEW world!
		m_PhysicsWorld->setEventListener(&m_PhysicsEventListener);
		RefreshPhysicsWorld();

		// Recreate the camera sensor in the new world (old one was destroyed above).
		InitCameraSensor();
	}

	RaycastData PhysicsSystem::CastRay(const Vector3f& startPoint, const Vector3f& endPoint, Filter filter /* = FilterPreset::All */)
	{
		rp3d::Vector3 start(startPoint.x, startPoint.y, startPoint.z);
		rp3d::Vector3 end(endPoint.x, endPoint.y, endPoint.z);
		rp3d::Ray ray(start, end);

		RaycastCallback callback;
		m_PhysicsWorld->raycast(ray, &callback, filter);

		RaycastData ret;
		ret.Hit = callback.HasHit();

		if (ret.Hit)
		{
			const RaycastInfoWrapper& info = callback.GetInfo();

			ret.HitFraction = info.hitFraction;
			ret.CollisionPoint = { info.worldPoint.x, info.worldPoint.y, info.worldPoint.z };
			ret.SurfaceNormal = { info.worldNormal.x, info.worldNormal.y, info.worldNormal.z };

			// Extract the Entity IDs
			EntityID rbID = static_cast<EntityID>(reinterpret_cast<uintptr_t>(info.body->getUserData()));

			// Extract the collider entity ID from the raw user-data entity value.
			EntityID collID = Constants::Entities::InvalidEntityID;
			if (info.collider->getUserData() != nullptr)
			{
				collID = static_cast<EntityID>(reinterpret_cast<uintptr_t>(info.collider->getUserData()));
			}

			ret.RigidBodyEntity = rbID;
			ret.ColliderEntity = collID;
		}

		return ret;
	}

	RaycastData PhysicsSystem::CastRay(const Vector3f& startPoint, const Vector3f& direction, float length, Filter filter /* = FilterPreset::All */)
	{
		Vector3f endPoint = startPoint + glm::normalize(direction) * length;
		return CastRay(startPoint, endPoint, filter);
	}

	OverlapTestData PhysicsSystem::TestOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, Filter filter /* = CollisionFilterPreset::All */)
	{
		RigidBodyComponent* rb = nullptr;
		if (entity != Constants::Entities::InvalidEntityID)
			rb = &entity.GetComponent<RigidBodyComponent>();

		// Create a temporary invisible KINEMATIC RigidBody at the target position
		rp3d::Vector3 halfExtents(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f);
		Quaternion rotationQuat = Math::ToQuaternion(rotation);

		rp3d::Transform transform(
			rp3d::Vector3(position.x, position.y, position.z),
			rp3d::Quaternion(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w)
		);

		rp3d::RigidBody* dummyBody = m_PhysicsWorld->createRigidBody(transform);
		dummyBody->setType(rp3d::BodyType::KINEMATIC);

		// Create the temporary sphere shape and attach it
		rp3d::BoxShape* boxShape = m_PhysicsCommon->createBoxShape(halfExtents);
		rp3d::Collider* collider = dummyBody->addCollider(boxShape, rp3d::Transform::identity());

		// Make it a trigger so it doesn't physically push objects away during the test
		collider->setIsTrigger(true);

		// Apply collision filters
		collider->setCollisionCategoryBits(FilterPreset::All);
		collider->setCollideWithMaskBits(filter);

		// Run the test
		reactphysics3d::RigidBody* rbBody = rb ? rb->Body : nullptr;
		OverlapTestCallback callback(rbBody, dummyBody);
		m_PhysicsWorld->testOverlap(dummyBody, callback);

		// Clean up the memory instantly
		dummyBody->removeCollider(collider);
		m_PhysicsCommon->destroyBoxShape(boxShape);
		m_PhysicsWorld->destroyRigidBody(dummyBody);

		return callback.GetOverlapData();
	}

	OverlapTestData PhysicsSystem::TestOverlapSphere(const Vector3f& position, float radius, Entity entity, Filter filter /* = CollisionFilterPreset::All */)
	{
		RigidBodyComponent* rb = nullptr;
		if (entity != Constants::Entities::InvalidEntityID)
			rb = &entity.GetComponent<RigidBodyComponent>();

		// Create a temporary invisible KINEMATIC RigidBody at the target position
		rp3d::Vector3 pos(position.x, position.y, position.z);
		rp3d::Transform transform(pos, rp3d::Quaternion::identity());

		rp3d::RigidBody* dummyBody = m_PhysicsWorld->createRigidBody(transform);
		dummyBody->setType(rp3d::BodyType::KINEMATIC);

		// Create the temporary sphere shape and attach it
		rp3d::SphereShape* sphereShape = m_PhysicsCommon->createSphereShape(radius);
		rp3d::Collider* collider = dummyBody->addCollider(sphereShape, rp3d::Transform::identity());

		// Make it a trigger so it doesn't physically push objects away during the test
		collider->setIsTrigger(true);

		// Apply collision filters
		collider->setCollisionCategoryBits(FilterPreset::All);
		collider->setCollideWithMaskBits(filter);

		// Run the test
		reactphysics3d::RigidBody* rbBody = rb ? rb->Body : nullptr;
		OverlapTestCallback callback(rbBody, dummyBody);
		m_PhysicsWorld->testOverlap(dummyBody, callback);

		// Clean up the memory instantly
		dummyBody->removeCollider(collider);
		m_PhysicsCommon->destroySphereShape(sphereShape);
		m_PhysicsWorld->destroyRigidBody(dummyBody);

		return callback.GetOverlapData();
	}

	CollisionCallbackData PhysicsSystem::TestCollision(Entity entity)
	{
		if (!entity.ContainsComponent<RigidBodyComponent>())
		{
			EB_CORE_ASSERT(false, "TestCollision called on an entity without a RigidBodyComponent!");
			return {};
		}

		RigidBodyComponent& rb = entity.GetComponent<RigidBodyComponent>();

		// Test overlapping colliders to filter out triggers before narrowphase collision checks
		TriggerFilterOverlapCallback triggerCallback(rb.Body);
		m_PhysicsWorld->testOverlap(rb.Body, triggerCallback);

		auto overlappingColliders = triggerCallback.GetSolidBodies();

		CollisionTestCallback callback(rb.Body);
		for (rp3d::Body* solidBody : overlappingColliders)
		{
			m_PhysicsWorld->testCollision(rb.Body, solidBody,callback);
		}

		return callback.GetCollisionData();
	}

	std::vector<VolumeOverlapData> PhysicsSystem::GetOverlappingVolumes(const Vector3f& cameraPosition, Filter overlapFilter)
	{
		// 1. Teleport the persistent camera sensor to the active camera position
		rp3d::Transform transform(
			rp3d::Vector3(cameraPosition.x, cameraPosition.y, cameraPosition.z),
			rp3d::Quaternion::identity()
		);
		m_CameraSensorBody->setTransform(transform);

		// 2. Initialize the callback with the position (for math) and the body (to ignore)
		VolumeOverlapCallback callback(cameraPosition, m_CameraSensorBody, overlapFilter);

		// 3. Test the overlaps! Because of the collision mask we set during Init, 
		// this will instantly skip the ground, the walls, and the player.
		m_PhysicsWorld->testOverlap(m_CameraSensorBody, callback);

		// 4. Move the sensor far out of the world so it can't interfere with the
		// next physics step (kinematic bodies stay wherever setTransform left them).
		m_CameraSensorBody->setTransform(rp3d::Transform(rp3d::Vector3(0.0f, -99999.0f, 0.0f), rp3d::Quaternion::identity()));

		return callback.GetOverlaps();
	}
	
	void PhysicsSystem::InitCameraSensor()
	{
		// 1. Create a persistent Kinematic body sitting at the origin
		m_CameraSensorBody = m_PhysicsWorld->createRigidBody(rp3d::Transform::identity());
		m_CameraSensorBody->setType(rp3d::BodyType::KINEMATIC);

		// 2. Attach a tiny sphere to represent the camera lens
		m_CameraSensorShape = m_PhysicsCommon->createSphereShape(0.1f);
		rp3d::Collider* collider = m_CameraSensorBody->addCollider(m_CameraSensorShape, rp3d::Transform::identity());
		collider->setIsTrigger(true);

		// 3. CRITICAL: Set filters so this ONLY checks against VFX Volumes.
		// Category is left at rp3d's default (0x0001) so volumes whose mask is 0xFFFF will see it.
		// The mask is set to VFX only so the sensor ignores everything else (floor, player, etc).
		collider->setCollideWithMaskBits(FilterPreset::All);
	}
	
	void PhysicsSystem::CreateRigidBody(EntityID entity, TransformComponent& transform, RigidBodyComponent& rigidBody)
	{
		// Decompose the World Transform to safely strip away the scale
		Vector3f worldPos, worldRot, worldScale;
		Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

		// Convert the pure, unscaled Euler rotation to a Quaternion
		Quaternion rotation = Math::ToQuaternion(worldRot);

		// Pass the pure global data to ReactPhysics3D
		rp3d::Vector3 initPos(worldPos.x, worldPos.y, worldPos.z);
		rp3d::Quaternion initRot(rotation.x, rotation.y, rotation.z, rotation.w);

		auto rp3dRigidBody = m_PhysicsWorld->createRigidBody(rp3d::Transform(initPos, initRot));
		rp3dRigidBody->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));
		rp3dRigidBody->setType(ToRp3dBodyType(rigidBody.Type));
		rp3dRigidBody->enableGravity(rigidBody.GravityEnabled);
		rp3dRigidBody->setIsDebugEnabled(m_DebugRenderSettings.Enabled);
		rp3dRigidBody->setIsAllowedToSleep(false);

		rigidBody.Body = rp3dRigidBody;
	}

	void PhysicsSystem::CreateBoxCollider(EntityID entity, BoxColliderComponent& box, Scene* scene)
	{
		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(entity, scene, ctx))
			return;

		rp3d::Vector3 extents(
			(box.Size.x * ctx.ChildWorldScale.x) * 0.5f,
			(box.Size.y * ctx.ChildWorldScale.y) * 0.5f,
			(box.Size.z * ctx.ChildWorldScale.z) * 0.5f
		);

		if (extents.x <= 0.0f || extents.y <= 0.0f || extents.z <= 0.0f)
		{
			EB_CORE_ERROR("Box Collider extents are zero!");
			extents = rp3d::Vector3(0.5f, 0.5f, 0.5f);
		}

		AttachAndUpdateMass(entity, box, m_PhysicsCommon->createBoxShape(extents), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, box.Offset));
		box.CachedWorldScale = ctx.ChildWorldScale;
		box.CachedWorldTransform = ctx.ChildWorldTransform;
	}

	void PhysicsSystem::CreateSphereCollider(EntityID entity, SphereColliderComponent& sphere, Scene* scene)
	{
		// TODO: See if user data gets set for collider (i.e. entityID)
		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(entity, scene, ctx))
			return;

		float maxScale = std::max({ ctx.ChildWorldScale.x, ctx.ChildWorldScale.y, ctx.ChildWorldScale.z });
		float radius = sphere.Radius * maxScale;
		if (radius <= 0.0f)
		{
			EB_CORE_ERROR("Sphere Collider radius is zero!");
			radius = 0.5f;
		}

		//auto physicsShape = m_PhysicsCommon->createSphereShape(radius);
		AttachAndUpdateMass(entity, sphere, m_PhysicsCommon->createSphereShape(radius), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, sphere.Offset));
		sphere.CachedWorldScale = ctx.ChildWorldScale;
		sphere.CachedWorldTransform = ctx.ChildWorldTransform;
	}

	void PhysicsSystem::CreateCapsuleCollider(EntityID entity, CapsuleColliderComponent& capsule, Scene* scene)
	{
		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(entity, scene, ctx))
			return;

		float maxScale = std::max({ ctx.ChildWorldScale.x, ctx.ChildWorldScale.y, ctx.ChildWorldScale.z });
		float radius = capsule.Radius * maxScale;
		float height = capsule.Height * maxScale;
		if (radius <= 0.0f)
		{
			EB_CORE_ERROR("Capsule Collider radius is zero!");
			radius = 0.5f;
		}
		if (height <= 0.0f)
		{
			EB_CORE_ERROR("Capsule Collider height is zero!");
			height = 2.0f;
		}

		// rp3d's height is the cylindrical section only (caps are added separately),
		// but CapsuleColliderComponent.Height is total height (matching the mesh generator).
		float cylinderHeight = std::max(0.01f, height - 2.0f * radius);

		AttachAndUpdateMass(entity, capsule, m_PhysicsCommon->createCapsuleShape(radius, cylinderHeight), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, capsule.Offset));
		capsule.CachedWorldScale = ctx.ChildWorldScale;
		capsule.CachedWorldTransform = ctx.ChildWorldTransform;
	}

	void PhysicsSystem::CreateConvexMeshCollider(EntityID entity, ConvexMeshColliderComponent& mesh, Scene* scene)
	{
		if (mesh.MeshHandle == Constants::InvalidUUID)
			return;

		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(entity, scene, ctx))
			return;

		auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
		if (!meshAsset)
			return;

		mesh.PhysicsVertices = meshAsset->GetVertexPositions();

		mesh.RP3DVertexArray = new rp3d::VertexArray(mesh.PhysicsVertices.data(), 3 * sizeof(float), meshAsset->GetVertexCount(), rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE);

		std::vector<rp3d::Message> messages;
		rp3d::ConvexMesh* convexMesh = m_PhysicsCommon->createConvexMesh(*mesh.RP3DVertexArray, messages);

		rp3d::Vector3 scaling(
			ctx.ChildWorldScale.x,
			ctx.ChildWorldScale.y,
			ctx.ChildWorldScale.z
		);
		AttachAndUpdateMass(entity, mesh, m_PhysicsCommon->createConvexMeshShape(convexMesh, scaling), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, mesh.Offset));
		mesh.CachedWorldScale = ctx.ChildWorldScale;
		mesh.CachedWorldTransform = ctx.ChildWorldTransform;
	}

	void PhysicsSystem::CreateConcaveMeshCollider(EntityID entity, ConcaveMeshColliderComponent& mesh, Scene* scene)
	{
		if (mesh.MeshHandle == Constants::InvalidUUID)
			return;

		ColliderSetupCtx ctx;
		if (!ResolveColliderSetup(entity, scene, ctx))
			return;

		auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
		if (!meshAsset)
			return;

		mesh.PhysicsVertices = meshAsset->GetVertexPositions();
		mesh.PhysicsIndices = meshAsset->GetTriangles();
		mesh.TriangleArray = new rp3d::TriangleVertexArray(
			meshAsset->GetVertexCount(), mesh.PhysicsVertices.data(), 3 * sizeof(float),
			meshAsset->GetTriangleCount(), mesh.PhysicsIndices.data(), 3 * sizeof(uint32_t),
			rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
			rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE
		);

		std::vector<rp3d::Message> messages;
		mesh.TriangleMesh = m_PhysicsCommon->createTriangleMesh(*mesh.TriangleArray, messages);

		rp3d::Vector3 scaling(
			ctx.ChildWorldScale.x,
			ctx.ChildWorldScale.y,
			ctx.ChildWorldScale.z
		);
		AttachAndUpdateMass(entity, mesh, m_PhysicsCommon->createConcaveMeshShape(mesh.TriangleMesh, scaling), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, mesh.Offset));
		mesh.CachedWorldScale = ctx.ChildWorldScale;
		mesh.CachedWorldTransform = ctx.ChildWorldTransform;
	}

	void PhysicsSystem::UpdateRigidbodies(Scene* scene)
	{
		// Dynamic:   physics drives the entity  (rp3d → local transform)
		// Kinematic: entity drives physics      (WorldTransform → rp3d)
		// Static:    no movement, no sync needed

		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<RigidBodyComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [rb, transform] = registry.GetComponents<RigidBodyComponent, TransformComponent>(entity);

			if (rb.Body == nullptr)
				continue;

			if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
			{
				// Physics drives the entity — read world transform from rp3d and write to
				// the entity's local transform fields (correct for root-level rigid bodies)
				const rp3d::Transform& rp3dTransform = rb.Body->getTransform();
				const rp3d::Vector3& pos = rp3dTransform.getPosition();
				const rp3d::Quaternion& rot = rp3dTransform.getOrientation();

				transform.Position = { pos.x, pos.y, pos.z };
				Quaternion rotation(rot.w, rot.x, rot.y, rot.z);
				transform.Rotation = Math::ToEulerAngles(rotation);
			}
			else if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
			{
				// Entity drives physics — push the entity's current world transform into
				// the rp3d body so the physics body follows the entity, not the reverse.
				Vector3f worldPos, worldRot, worldScale;
				Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

				Quaternion q = Math::ToQuaternion(worldRot);
				rb.Body->setTransform(rp3d::Transform(
					rp3d::Vector3(worldPos.x, worldPos.y, worldPos.z),
					rp3d::Quaternion(q.x, q.y, q.z, q.w)
				));
			}
		}
	}

	void PhysicsSystem::UpdateAvoidanceCollisions(Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<LocalAvoidanceComponent, TransformComponent>();
		for (EntityID entityId : view)
		{
			Entity entity(entityId, scene);
			auto [avoidance, transform] = registry.GetComponents<LocalAvoidanceComponent, TransformComponent>(entity);

			Vector3f myPos = transform.GetWorldPosition();
			Vector3f totalRepelVector = { 0, 0, 0 };
			int neighborCount = 0;

			// Use your Physics System to only check things within the radius
			auto overlaps = TestOverlapSphere(myPos, avoidance.AvoidanceRadius, entity, avoidance.AvoidanceMask);
			for (const auto& hit : overlaps.Hits)
			{
				if (hit.EntityID == entityId)
					continue; // Don't run away from ourselves (shouldn't happen due to TestOverlapSphere entity param)

				Entity hitEntity(hit.EntityID, scene);
				Vector3f neighborPos = hitEntity.GetComponent<TransformComponent>().GetWorldPosition();

				// Calculate vector pointing AWAY from the neighbor
				Vector3f repelDir = myPos - neighborPos;
				float distance = Math::Length(repelDir);

				if (distance > 0.001f && distance < avoidance.AvoidanceRadius)
				{
					repelDir = Math::Normalize(repelDir);

					// The closer they are, the harder we push away
					float forceMultiplier = 1.0f - (distance / avoidance.AvoidanceRadius);
					totalRepelVector += (repelDir * forceMultiplier);
					neighborCount++;
				}
			}

			if (neighborCount > 0)
			{
				// Average the vector and apply the component's force multiplier
				totalRepelVector = totalRepelVector / (float)neighborCount;
				avoidance.AvoidanceVector = totalRepelVector * avoidance.AvoidanceStrength;
			}
			else
			{
				avoidance.AvoidanceVector = { 0, 0, 0 };
			}
		}
	}

	void PhysicsSystem::UpdateScriptTriggers(Scene* scene)
	{
		auto& overlapTriggers = m_PhysicsEventListener.GetOverlapData();
		for (const auto& triggerEvent : overlapTriggers)
		{
			// Fire trigger event for both entities
			ScriptSystem::FireTriggerEvent(triggerEvent.EntityA, triggerEvent.EntityB, triggerEvent.EventType, scene);
			ScriptSystem::FireTriggerEvent(triggerEvent.EntityB, triggerEvent.EntityA, triggerEvent.EventType, scene);
		}

		m_PhysicsEventListener.ClearOverlapQueue();
	}

	void PhysicsSystem::ShowDebugRendererIfApplicable()
	{
		if (m_PhysicsWorld)
		{
			bool hasPostProcessDebugEntity = m_PostProcessDebugEntity != Constants::Entities::InvalidEntityID;
			bool debugActive = m_DebugRenderSettings.Enabled || hasPostProcessDebugEntity;

			m_PhysicsWorld->setIsDebugRenderingEnabled(debugActive);

			if (debugActive)
			{
				auto& debugRenderer = m_PhysicsWorld->getDebugRenderer();
				debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, m_DebugRenderSettings.DrawColliders || hasPostProcessDebugEntity);
				debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, m_DebugRenderSettings.DrawContactPoints);
				debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, m_DebugRenderSettings.DrawColliderAxes);
			}
		}
	}

	void PhysicsSystem::UpdateDebugRenderData()
	{
		// Regenerate debug primitives each frame so GetDebugLines/GetDebugLineCount return current data
		auto& debugRenderer = m_PhysicsWorld->getDebugRenderer();
		debugRenderer.reset();

		if (m_DebugRenderSettings.Enabled || m_PostProcessDebugEntity != Constants::Entities::InvalidEntityID)
		{
			debugRenderer.computeDebugRenderingPrimitives(*m_PhysicsWorld);

			// Unpack Color Helper
			auto unpackColor = [](uint32_t color) -> Vector4f {
				return Vector4f(((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f, (color & 0xFF) / 255.0f, 1.0f);
			};

			// Push Lines
			uint32_t lineCount = debugRenderer.getNbLines();
			if (lineCount > 0)
			{
				const auto* lines = debugRenderer.getLinesArray();
				for (uint32_t i = 0; i < lineCount; i++)
				{
					Vector3f point1 = { lines[i].point1.x, lines[i].point1.y, lines[i].point1.z };
					Vector3f point2 = { lines[i].point2.x, lines[i].point2.y, lines[i].point2.z };
					DebugRenderer::DrawLine(point1, point2, unpackColor(lines[i].color1));
				}
			}

			// Push Triangles
			uint32_t triCount = debugRenderer.getNbTriangles();
			if (triCount > 0)
			{
				const auto* tris = debugRenderer.getTrianglesArray();
				for (uint32_t i = 0; i < triCount; i++)
				{
					Vector3f point1 = { tris[i].point1.x, tris[i].point1.y, tris[i].point1.z };
					Vector3f point2 = { tris[i].point2.x, tris[i].point2.y, tris[i].point2.z };
					Vector3f point3 = { tris[i].point3.x, tris[i].point3.y, tris[i].point3.z };
					DebugRenderer::DrawTriangle(point1, point2, point3, unpackColor(tris[i].color1));
				}
			}
		}
	}

}