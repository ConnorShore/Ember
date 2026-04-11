#pragma once

#include "Ember/ECS/Types.h"
#include "System.h"
#include "Ember/ECS/Component/Components.h"

#include <reactphysics3d/reactphysics3d.h>

namespace Ember {

	struct PhysicsSettings
	{
		float Gravity = -9.81f;
		uint32_t UpdateRate = 60;

		uint32_t PositionSolverIterations = 5;
		uint32_t VelocitySolverIterations = 10;
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

		PhysicsSettings& GetSettings() { return m_Settings; }
		void RefreshPhysicsWorld();

	private:
		void CreateRigidBody(EntityID entity, TransformComponent& transform, RigidBodyComponent& rigidBody);

	private:
		rp3d::PhysicsCommon m_PhysicsCommon;
		rp3d::PhysicsWorld* m_PhysicsWorld = nullptr;

		PhysicsSettings m_Settings;

		float m_TimeAcumulator = 0.0f;
	};

}