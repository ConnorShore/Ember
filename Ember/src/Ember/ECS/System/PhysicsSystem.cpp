#include "ebpch.h"
#include "PhysicsSystem.h"
#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Render/DebugRenderer.h"

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

	static rp3d::Transform MakeColliderTransform(const Vector3f& relPos, const Vector3f& relRot, const Vector3f& offset = Vector3f(0.0f))
	{
		Quaternion q = Math::ToQuaternion(relRot);
		return rp3d::Transform(
			rp3d::Vector3(relPos.x + offset.x, relPos.y + offset.y, relPos.z + offset.z),
			rp3d::Quaternion(q.x, q.y, q.z, q.w)
		);
	}

	template<typename TCollider, typename TShape>
	static void AttachAndUpdateMass(TCollider& collider, TShape* shape, RigidBodyComponent& rb, const rp3d::Transform& localTransform)
	{
		collider.Shape = shape;
		collider.Collider = rb.Body->addCollider(shape, localTransform);
		collider.AttachedBody = rb.Body;

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
		EB_CORE_INFO("Physics System attached!");
	}

	void PhysicsSystem::OnDetach()
	{
		EB_CORE_INFO("Physics System detached!");
	}

	void PhysicsSystem::OnSceneAttach(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		m_PhysicsCommon = ScopedPtr<rp3d::PhysicsCommon>::Create();

		m_PhysicsWorld = m_PhysicsCommon->createPhysicsWorld();
		RefreshPhysicsWorld();

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

				AttachAndUpdateMass(box, m_PhysicsCommon->createBoxShape(extents), *ctx.Rb,
					MakeColliderTransform(ctx.RelPos, ctx.RelRot, box.Offset));
			}
		);

		registry.ConnectAndRetroact<SphereColliderComponent>(
			[this, scene](EntityID entity, SphereColliderComponent& sphere) {
				if (sphere.Shape != nullptr)
					return;

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

				AttachAndUpdateMass(sphere, m_PhysicsCommon->createSphereShape(radius), *ctx.Rb,
					MakeColliderTransform(ctx.RelPos, ctx.RelRot, sphere.Offset));
			}
		);

		registry.ConnectAndRetroact<ConvexMeshColliderComponent>(
			[this, scene](EntityID entity, ConvexMeshColliderComponent& mesh) {
				if (mesh.Shape != nullptr)
					return;
				ColliderSetupCtx ctx;
				if (!ResolveColliderSetup(entity, scene, ctx))
					return;
				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				float maxScale = std::max({ ctx.ChildWorldScale.x, ctx.ChildWorldScale.y, ctx.ChildWorldScale.z });
				mesh.PhysicsVertices = meshAsset->GetVertexPositions();

				mesh.RP3DVertexArray = new rp3d::VertexArray(mesh.PhysicsVertices.data(), 3 * sizeof(float), meshAsset->GetVertexCount(), rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE);

				std::vector<rp3d::Message> messages;
				rp3d::ConvexMesh* convexMesh = m_PhysicsCommon->createConvexMesh(*mesh.RP3DVertexArray, messages);

				rp3d::Vector3 scaling(maxScale, maxScale, maxScale);
				AttachAndUpdateMass(mesh, m_PhysicsCommon->createConvexMeshShape(convexMesh, scaling), *ctx.Rb,
					MakeColliderTransform(ctx.RelPos, ctx.RelRot));
			}
		);

		registry.ConnectAndRetroact<ConcaveMeshColliderComponent>(
			[this, scene](EntityID entity, ConcaveMeshColliderComponent& mesh) {
				if (mesh.Shape != nullptr)
					return;

				ColliderSetupCtx ctx;
				if (!ResolveColliderSetup(entity, scene, ctx))
					return;

				auto meshAsset = Application::Instance().GetAssetManager().GetAsset<Mesh>(mesh.MeshHandle);
				float maxScale = std::max({ ctx.ChildWorldScale.x, ctx.ChildWorldScale.y, ctx.ChildWorldScale.z });

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

				rp3d::Vector3 scaling(maxScale, maxScale, maxScale);
				AttachAndUpdateMass(mesh, m_PhysicsCommon->createConcaveMeshShape(mesh.TriangleMesh, scaling), *ctx.Rb,
					MakeColliderTransform(ctx.RelPos, ctx.RelRot));
			}
		);

		registry.ConnectAndRetroact<CapsuleColliderComponent>(
			[this, scene](EntityID entity, CapsuleColliderComponent& capsule) {
				if (capsule.Shape != nullptr)
					return;

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

				AttachAndUpdateMass(capsule, m_PhysicsCommon->createCapsuleShape(radius, height), *ctx.Rb,
					MakeColliderTransform(ctx.RelPos, ctx.RelRot, capsule.Offset));
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
		m_PhysicsCommon->destroyPhysicsWorld(m_PhysicsWorld);
		m_PhysicsWorld = nullptr;
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

		// Sync Physics -> ECS (Update graphics transforms)
		auto& registry = scene->GetRegistry();
		auto view = registry.Query<RigidBodyComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [rb, transform] = registry.GetComponents<RigidBodyComponent, TransformComponent>(entity);

			if (rb.Body != nullptr)
			{
				const rp3d::Transform& rp3dTransform = rb.Body->getTransform();
				const rp3d::Vector3& pos = rp3dTransform.getPosition();
				const rp3d::Quaternion& rot = rp3dTransform.getOrientation();

				transform.Position = { pos.x, pos.y, pos.z };

				Quaternion rotation(rot.w, rot.x, rot.y, rot.z);
				transform.Rotation = Math::ToEulerAngles(rotation);
			}
		}

		UpdateDebugRenderData();
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
		rp3dRigidBody->setType(ToRp3dBodyType(rigidBody.Type));
		rp3dRigidBody->enableGravity(rigidBody.GravityEnabled);
		rp3dRigidBody->setIsDebugEnabled(m_DebugRenderSettings.Enabled);
		rp3dRigidBody->setIsAllowedToSleep(false);

		rigidBody.Body = rp3dRigidBody;
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