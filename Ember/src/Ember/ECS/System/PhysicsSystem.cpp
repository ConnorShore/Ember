#include "ebpch.h"
#include "PhysicsSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

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

		m_PhysicsWorld = m_PhysicsCommon.createPhysicsWorld();
		RefreshPhysicsWorld();

		// Handles both Future Spawns AND the Backfill!
		scene->GetRegistry().ConnectAndRetroact<RigidBodyComponent>(
			[this, scene](EntityID entity, RigidBodyComponent& rb) {

				// Only create the body if it doesn't exist yet!
				if (rb.Body == nullptr)
				{
					auto& transform = scene->GetRegistry().GetComponent<TransformComponent>(entity);
					this->CreateRigidBody(entity, transform, rb);
				}
			}
		);

		// Detach doesn't need to be retroactive, just standard Connect
		scene->GetRegistry().OnComponentDetached<RigidBodyComponent>().Connect(
			[this](EntityID entity, RigidBodyComponent& rb) {
				if (rb.Body) {
					m_PhysicsWorld->destroyRigidBody(rb.Body);
					rb.Body = nullptr;
				}
			}
		);
	}

	void PhysicsSystem::OnSceneDetach(Scene* scene)
	{
		m_PhysicsCommon.destroyPhysicsWorld(m_PhysicsWorld);
		m_PhysicsWorld = nullptr;
	}

	// Simple explicit Euler integration: position += velocity * dt
	void PhysicsSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		const float timeStep = 1.0f / m_Settings.UpdateRate;
		m_TimeAcumulator += delta;

		// Step the physics simulation
		while (m_TimeAcumulator >= timeStep) {
			m_PhysicsWorld->update(timeStep);
			m_TimeAcumulator -= timeStep;
		}

		// Sync Physics -> ECS (Update graphics transforms)
		auto view = scene->GetRegistry().Query<RigidBodyComponent, TransformComponent>();
		for (EntityID entity : view)
		{
			auto [rb, transform] = scene->GetRegistry().GetComponents<RigidBodyComponent, TransformComponent>(entity);

			if (rb.Body != nullptr)
			{
				const rp3d::Transform& rp3dTransform = rb.Body->getTransform();
				const rp3d::Vector3& pos = rp3dTransform.getPosition();
				const rp3d::Quaternion& rot = rp3dTransform.getOrientation();

				transform.Position = { pos.x, pos.y, pos.z };

				Quaternion rotation(rot.x, rot.y, rot.z, rot.w);
				transform.Rotation = Math::ToEulerAngles(rotation);
			}
		}
	}

	void PhysicsSystem::RefreshPhysicsWorld()
	{
		m_PhysicsWorld->setGravity(rp3d::Vector3(0, m_Settings.Gravity, 0));
		m_PhysicsWorld->setNbIterationsPositionSolver(m_Settings.PositionSolverIterations);
		m_PhysicsWorld->setNbIterationsVelocitySolver(m_Settings.VelocitySolverIterations);

		// TODO: Verify that the physics world is properly updated with the new settings
		// or if world needs to be recreated
	}

	void PhysicsSystem::CreateRigidBody(EntityID entity, TransformComponent& transform, RigidBodyComponent& rigidBody)
	{
		Quaternion rotation = Math::ToQuaternion(transform.WorldTransform);

		rp3d::Vector3 initPos(transform.Position.x, transform.Position.y, transform.Position.z);
		rp3d::Quaternion initRot(rotation.x, rotation.y, rotation.z, rotation.w);

		auto rp3dRigidBody = m_PhysicsWorld->createRigidBody(rp3d::Transform(initPos, initRot));
		rp3dRigidBody->setType(ToRp3dBodyType(rigidBody.Type));
		rp3dRigidBody->enableGravity(rigidBody.GravityEnabled);
		rp3dRigidBody->setMass(rigidBody.Mass);

		rigidBody.Body = rp3dRigidBody;
	}

}