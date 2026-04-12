#include "ebpch.h"
#include "PhysicsSystem.h"
#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"

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


	// --- PHYSICS SYSTEM IMPLEMENTATION ---

	PhysicsSystem::PhysicsSystem()
	{
	}

	PhysicsSystem::~PhysicsSystem()
	{
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

				// Idempotent check
				if (rb.Body == nullptr)
				{
					auto& transform = scene->GetRegistry().GetComponent<TransformComponent>(entity);
					this->CreateRigidBody(entity, transform, rb);
				}
			}
		);

		registry.ConnectAndRetroact<BoxColliderComponent>(
			[this, scene](EntityID entity, BoxColliderComponent& box) {

				// Idempotent check
				if (box.Shape == nullptr)
				{
					EntityID rootBodyEntity = FindRigidBodyEntity(entity, scene);

					if (rootBodyEntity != Constants::Entities::InvalidEntityID)
					{
						auto& rb = scene->GetRegistry().GetComponent<RigidBodyComponent>(rootBodyEntity);
						auto& rootTransform = scene->GetRegistry().GetComponent<TransformComponent>(rootBodyEntity);
						auto& childTransform = scene->GetRegistry().GetComponent<TransformComponent>(entity);

						if (rb.Body != nullptr)
						{
							// Get the transform of the child relative to the Root RigidBody
							Matrix4f relativeMatrix = Math::Inverse(rootTransform.WorldTransform) * childTransform.WorldTransform;

							Vector3f relPos, relRot, relScale;
							Math::DecomposeTransform(relativeMatrix, relPos, relRot, relScale);

							// Use the child's absolute world scale for extents so the collider always
							// matches the visual size (relScale is identity when collider == body entity)
							Vector3f childWorldPos, childWorldRot, childWorldScale;
							Math::DecomposeTransform(childTransform.WorldTransform, childWorldPos, childWorldRot, childWorldScale);

							// Calculate the box extents (half-sizes) and ensure they are not zero
							rp3d::Vector3 extents((box.Size.x * childWorldScale.x) * 0.5f, (box.Size.y * childWorldScale.y) * 0.5f, (box.Size.z * childWorldScale.z) * 0.5f);
							if (extents.x <= 0.0f || extents.y <= 0.0f || extents.z <= 0.0f) {
								EB_CORE_ERROR("Box Collider extents are zero!");
								extents = rp3d::Vector3(0.5f, 0.5f, 0.5f);
							}

							// Scale the box by the relative hierarchy scale
							box.Shape = m_PhysicsCommon->createBoxShape(extents);

							// Position the box at the relative hierarchy offset
							Quaternion localRotation = Math::ToQuaternion(relRot);
							rp3d::Transform rp3dLocal(
								rp3d::Vector3(relPos.x + box.Offset.x, relPos.y + box.Offset.y, relPos.z + box.Offset.z),
								rp3d::Quaternion(localRotation.x, localRotation.y, localRotation.z, localRotation.w)
							);

							// Attach to the body and save references for cleanup
							box.Collider = rb.Body->addCollider(box.Shape, rp3dLocal);
							box.AttachedBody = rb.Body;

							// Update the RigidBody's mass properties to account for the new collider
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
										rb.Body->setLocalInertiaTensor(localInertia * massRatio); // Scale it!
									}
								}
							}
						}
					}
				}
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

				// Safely remove the collider from the RigidBody before destroying the shape!
				if (box.Collider && box.AttachedBody) 
				{
					box.AttachedBody->removeCollider(box.Collider);

					if (box.Shape)
						m_PhysicsCommon->destroyBoxShape(box.Shape);

					box.Collider = nullptr;
					box.Shape = nullptr;
					box.AttachedBody = nullptr;
				}
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

		// Regenerate debug primitives each frame so GetDebugLines/GetDebugLineCount return current data
		auto& debugRenderer = m_PhysicsWorld->getDebugRenderer();
		debugRenderer.reset();
		if (m_DebugRenderSettings.Enabled)
			debugRenderer.computeDebugRenderingPrimitives(*m_PhysicsWorld);
	}

	void PhysicsSystem::RefreshPhysicsWorld()
	{
		m_PhysicsWorld->setGravity(rp3d::Vector3(0, m_Settings.Gravity, 0));
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

	const DebugLine* PhysicsSystem::GetDebugLines() const
	{
		if (m_PhysicsWorld == nullptr || GetDebugLineCount() == 0)
			return nullptr;

		// Cast the raw memory so the rest of the engine never sees rp3d
		const auto* rp3dLines = m_PhysicsWorld->getDebugRenderer().getLinesArray();
		return reinterpret_cast<const DebugLine*>(rp3dLines);
	}

	uint32_t PhysicsSystem::GetDebugLineCount() const
	{
		if (m_PhysicsWorld == nullptr)
			return 0;

		return m_PhysicsWorld->getDebugRenderer().getNbLines();
	}

	const DebugTriangle* PhysicsSystem::GetDebugTriangles() const
	{
		if (m_PhysicsWorld == nullptr || GetDebugTriangleCount() == 0)
			return nullptr;

		const auto* rp3dTriangles = m_PhysicsWorld->getDebugRenderer().getTrianglesArray();
		return reinterpret_cast<const DebugTriangle*>(rp3dTriangles);
	}

	uint32_t PhysicsSystem::GetDebugTriangleCount() const
	{
		if (m_PhysicsWorld == nullptr)
			return 0;

		return m_PhysicsWorld->getDebugRenderer().getNbTriangles();
	}

}