#include "ebpch.h"
#include "PhysicsSystem.h"

#include "Ember/Core/Core.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

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
		//View view = scene->GetRegistry().Query<RigidBodyComponent, TransformComponent>();
		//for (EntityID entity : view)
		//{
		//	auto [transform, rigidBody] = scene->GetRegistry().GetComponents<TransformComponent, RigidBodyComponent>(entity);
		//	transform.Position += rigidBody.Velocity * (float)delta;
		//}

		const float timeStep = 1.0f / m_Settings.UpdateRate;

		// Add the time difference in the accumulator
		m_TimeAcumulator += delta;

		// While there is enough accumulated time to take
		// one or several physics steps
		while (m_TimeAcumulator >= timeStep) {

			// Update the Dynamics world with a constant time step
			m_PhysicsWorld->update(timeStep);

			// Decrease the accumulated time
			m_TimeAcumulator -= timeStep;
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

		rigidBody.Body = m_PhysicsWorld->createRigidBody(rp3d::Transform(initPos, initRot));
	}

}