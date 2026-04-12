#pragma once

#include "Ember/ECS/Types.h"
#include "System.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Render/DebugLine.h"

// forward declarations
namespace reactphysics3d {
	class PhysicsCommon;
	class PhysicsWorld;
}
namespace rp3d = reactphysics3d;

namespace Ember {

	struct PhysicsSettings
	{
		float Gravity = -9.81f;
		uint32_t UpdateRate = 60;

		uint32_t PositionSolverIterations = 5;
		uint32_t VelocitySolverIterations = 10;
	};

	struct DebugRenderSettings
	{
		bool Enabled = true;
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

		PhysicsSettings& GetSettings() { return m_Settings; }
		void RefreshPhysicsWorld();

		DebugRenderSettings& GetDebugRenderSettings() { return m_DebugRenderSettings; }

		const DebugLine* GetDebugLines() const;
		uint32_t GetDebugLineCount() const;

		const DebugTriangle* GetDebugTriangles() const;
		uint32_t GetDebugTriangleCount() const;

	private:
		void CreateRigidBody(EntityID entity, TransformComponent& transform, RigidBodyComponent& rigidBody);

	private:
		ScopedPtr<rp3d::PhysicsCommon> m_PhysicsCommon = nullptr;
		//rp3d::PhysicsCommon* m_PhysicsCommon = nullptr;
		rp3d::PhysicsWorld* m_PhysicsWorld = nullptr;

		PhysicsSettings m_Settings;
		DebugRenderSettings m_DebugRenderSettings;

		float m_TimeAcumulator = 0.0f;
	};

}