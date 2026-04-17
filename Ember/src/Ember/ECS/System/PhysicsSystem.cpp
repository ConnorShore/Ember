#include "ebpch.h"
#include "PhysicsSystem.h"
#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Render/DebugRenderer.h"
#include "Ember/Physics/RaycastCallback.h"
#include "Ember/Physics/OverlapTestCallback.h"

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
		collider.Collider->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

		// Set trigger
		collider.Collider->setIsTrigger(collider.IsTrigger);

		// Set collision filters
		if (collider.Category != CollisionFilterPreset::Default)
			collider.Collider->setCollisionCategoryBits(collider.Category);

		if (collider.CollisionMask != CollisionFilterPreset::Default)
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
			[this](EntityID entity, RigidBodyComponent& rb) {
				if (rb.Body) {
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
	}

	void PhysicsSystem::OnSceneDetach(Scene* scene)
	{
		//m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);
		//m_PhysicsWorld = nullptr;
	}

	void PhysicsSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		ShowDebugRendererIfApplicable();

		const float timeStep = 1.0f / m_Settings.UpdateRate;
		m_TimeAcumulator += delta;

		// Step the physics simulation
		while (m_TimeAcumulator >= timeStep)
		{
			m_PhysicsWorld->update(timeStep);
			m_TimeAcumulator -= timeStep;
		}

		// Dynamic:   physics drives the entity  (rp3d → local transform)
		// Kinematic: entity drives physics      (WorldTransform → rp3d)
		// Static:    no movement, no sync needed
		auto& registry = scene->GetRegistry();
		auto view = registry.Query<RigidBodyComponent, TransformComponent>();
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

		UpdateDebugRenderData();
	}

	void PhysicsSystem::OnEditorUpdate(TimeStep delta, Scene* scene)
	{
		// Sync ECS -> Physics (So when you drag objects with your mouse, the collider moves!)
		auto& registry = scene->GetRegistry();
		auto view = registry.Query<RigidBodyComponent, TransformComponent>();

		for (EntityID entity : view)
		{
			auto [rb, transform] = registry.GetComponents<RigidBodyComponent, TransformComponent>(entity);

			if (rb.Body != nullptr)
				{
					rb.Body->setIsDebugEnabled(m_DebugRenderSettings.Enabled);

					// Sync body type and gravity so inspector changes are reflected before play
					rb.Body->setType(ToRp3dBodyType(rb.Type));
					rb.Body->enableGravity(rb.GravityEnabled);

					// Take the TransformComponent and push it INTO ReactPhysics3D
					Vector3f worldPos, worldRot, worldScale;
					Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);

					rp3d::Vector3 newPos(worldPos.x, worldPos.y, worldPos.z);
					Quaternion q = Math::ToQuaternion(worldRot);
					rp3d::Quaternion newRot(q.x, q.y, q.z, q.w);

					rb.Body->setTransform(rp3d::Transform(newPos, newRot));
				}
		}

		// Rebuild colliders whose properties were changed in the inspector
		auto boxView = registry.Query<BoxColliderComponent>();
		for (EntityID entity : boxView)
		{
			auto& box = registry.GetComponent<BoxColliderComponent>(entity);
			if (box.NeedsRebuild)
			{
				DetachCollider(box, [&]() { m_PhysicsCommon->destroyBoxShape(box.Shape); });
				CreateBoxCollider(entity, box, scene);
				box.NeedsRebuild = false;
			}
		}

		auto sphereView = registry.Query<SphereColliderComponent>();
		for (EntityID entity : sphereView)
		{
			auto& sphere = registry.GetComponent<SphereColliderComponent>(entity);
			if (sphere.NeedsRebuild)
			{
				DetachCollider(sphere, [&]() { m_PhysicsCommon->destroySphereShape(sphere.Shape); });
				CreateSphereCollider(entity, sphere, scene);
				sphere.NeedsRebuild = false;
			}
		}

		auto capsuleView = registry.Query<CapsuleColliderComponent>();
		for (EntityID entity : capsuleView)
		{
			auto& capsule = registry.GetComponent<CapsuleColliderComponent>(entity);
			if (capsule.NeedsRebuild)
			{
				DetachCollider(capsule, [&]() { m_PhysicsCommon->destroyCapsuleShape(capsule.Shape); });
				CreateCapsuleCollider(entity, capsule, scene);
				capsule.NeedsRebuild = false;
			}
		}

		auto convexView = registry.Query<ConvexMeshColliderComponent>();
		for (EntityID entity : convexView)
		{
			auto& mesh = registry.GetComponent<ConvexMeshColliderComponent>(entity);
			if (mesh.NeedsRebuild)
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

		auto concaveView = registry.Query<ConcaveMeshColliderComponent>();
		for (EntityID entity : concaveView)
		{
			auto& mesh = registry.GetComponent<ConcaveMeshColliderComponent>(entity);
			if (mesh.NeedsRebuild)
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

		ShowDebugRendererIfApplicable();
		UpdateDebugRenderData();
	}

	void PhysicsSystem::RemoveRigidBody(RigidBodyComponent& rigidBody)
	{
		m_PhysicsWorld->destroyRigidBody(rigidBody.Body);
		rigidBody.Body = nullptr;
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
		m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);
		m_PhysicsWorld = m_PhysicsCommon->createPhysicsWorld();
	}

	RaycastData PhysicsSystem::CastRay(const Vector3f& startPoint, const Vector3f& endPoint)
	{
		rp3d::Vector3 start(startPoint.x, startPoint.y, startPoint.z);
		rp3d::Vector3 end(endPoint.x, endPoint.y, endPoint.z);
		rp3d::Ray ray(start, end);

		RaycastCallback callback;
		m_PhysicsWorld->raycast(ray, &callback);

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
			EntityID collID = static_cast<EntityID>(reinterpret_cast<uintptr_t>(info.collider->getUserData()));

			ret.RigidBodyEntity = rbID;
			ret.ColliderEntity = collID;
		}

		return ret;
	}

	bool PhysicsSystem::TestOverlapBox(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, CollisionFilter filter /* = CollisionFilterPreset::All */)
	{
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
		collider->setCollisionCategoryBits(CollisionFilterPreset::All);
		collider->setCollideWithMaskBits(filter);

		// Run the test
		OverlapTestCallback callback;
		m_PhysicsWorld->testOverlap(dummyBody, callback);

		// Clean up the memory instantly
		dummyBody->removeCollider(collider);
		m_PhysicsCommon->destroyBoxShape(boxShape);
		m_PhysicsWorld->destroyRigidBody(dummyBody);

		return callback.HasHit();
	}

	bool PhysicsSystem::TestOverlapSphere(const Vector3f& position, float radius, CollisionFilter filter /* = CollisionFilterPreset::All */, rp3d::RigidBody* bodyToIgnore /* = nullptr */)
	{
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
		collider->setCollisionCategoryBits(CollisionFilterPreset::All);
		collider->setCollideWithMaskBits(filter);

		// Run the test
		OverlapTestCallback callback(bodyToIgnore);
		m_PhysicsWorld->testOverlap(dummyBody, callback);

		// Clean up the memory instantly
		dummyBody->removeCollider(collider);
		m_PhysicsCommon->destroySphereShape(sphereShape);
		m_PhysicsWorld->destroyRigidBody(dummyBody);

		return callback.HasHit();
	}

	CollisionCallbackData PhysicsSystem::TestCollision(Entity entity)
	{
		if (!entity.ContainsComponent<RigidBodyComponent>())
		{
			EB_CORE_ASSERT(false, "TestCollision called on an entity without a RigidBodyComponent!");
			return {};
		}

		RigidBodyComponent& rb = entity.GetComponent<RigidBodyComponent>();

		// Pass the specific rigid body into the callback so we can fix the normal direction!
		CollisionTestCallback callback(rb.Body);
		m_PhysicsWorld->testCollision(rb.Body, callback);

		return callback.GetCollisionData();
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
	}

	void PhysicsSystem::CreateSphereCollider(EntityID entity, SphereColliderComponent& sphere, Scene* scene)
	{
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

		AttachAndUpdateMass(entity, sphere, m_PhysicsCommon->createSphereShape(radius), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, sphere.Offset));
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
		float cylinderHeight = std::max(0.0f, height - 2.0f * radius);

		AttachAndUpdateMass(entity, capsule, m_PhysicsCommon->createCapsuleShape(radius, cylinderHeight), *ctx.Rb,
			MakeColliderTransform(ctx.RelPos, ctx.RelRot, capsule.Offset));
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
	}

	void PhysicsSystem::ShowDebugRendererIfApplicable()
	{
		if (m_PhysicsWorld)
		{
			m_PhysicsWorld->setIsDebugRenderingEnabled(m_DebugRenderSettings.Enabled);

			if (m_DebugRenderSettings.Enabled)
			{
				auto& debugRenderer = m_PhysicsWorld->getDebugRenderer();
				debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, m_DebugRenderSettings.DrawColliders);
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

		if (m_DebugRenderSettings.Enabled)
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