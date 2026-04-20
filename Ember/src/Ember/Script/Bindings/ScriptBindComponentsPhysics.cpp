#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindPhysicsComponents(sol::state& state)
	{
		state.new_usertype<RigidBodyComponent>("RigidBodyComponent",
			"Mass", &RigidBodyComponent::Mass,
			"GravityEnabled", &RigidBodyComponent::GravityEnabled,
			"ApplyForce", &RigidBodyComponent::ApplyForce,
			"ApplyImpulse", &RigidBodyComponent::ApplyImpulse
		);

		state.new_usertype<ColliderOffset>("ColliderOffset",
			"Position", &ColliderOffset::Position,
			"Rotation", &ColliderOffset::Rotation
		);

		state.new_usertype<BoxColliderComponent>("BoxColliderComponent",
			"Size", &BoxColliderComponent::Size,
			"Offset", &BoxColliderComponent::Offset,
			"Category", &BoxColliderComponent::Category,
			"CollisionMask", &BoxColliderComponent::CollisionMask
		);

		state.new_usertype<SphereColliderComponent>("SphereColliderComponent",
			"Radius", &SphereColliderComponent::Radius,
			"Offset", &SphereColliderComponent::Offset,
			"Category", &SphereColliderComponent::Category,
			"CollisionMask", &SphereColliderComponent::CollisionMask
		);

		state.new_usertype<CapsuleColliderComponent>("CapsuleColliderComponent",
			"Radius", &CapsuleColliderComponent::Radius,
			"Height", &CapsuleColliderComponent::Height,
			"Offset", &CapsuleColliderComponent::Offset,
			"Category", &CapsuleColliderComponent::Category,
			"CollisionMask", &CapsuleColliderComponent::CollisionMask
		);

		state.new_usertype<ConvexMeshColliderComponent>("ConvexMeshColliderComponent",
			"MeshHandle", &ConvexMeshColliderComponent::MeshHandle,
			"Offset", &ConvexMeshColliderComponent::Offset,
			"Category", &ConvexMeshColliderComponent::Category,
			"CollisionMask", &ConvexMeshColliderComponent::CollisionMask
		);

		state.new_usertype<ConcaveMeshColliderComponent>("ConcaveMeshColliderComponent",
			"MeshHandle", &ConcaveMeshColliderComponent::MeshHandle,
			"Offset", &ConcaveMeshColliderComponent::Offset,
			"Category", &ConcaveMeshColliderComponent::Category,
			"CollisionMask", &ConcaveMeshColliderComponent::CollisionMask
		);

		state.new_usertype<CharacterControllerComponent>("CharacterControllerComponent",
			"WalkSpeed", &CharacterControllerComponent::WalkSpeed,
			"JumpForce", &CharacterControllerComponent::JumpForce,
			"GravityMultiplier", &CharacterControllerComponent::GravityMultiplier,
			"MaxSlopeAngle", &CharacterControllerComponent::MaxSlopeAngle,
			"MaxStepHeight", &CharacterControllerComponent::MaxStepHeight,
			"IsGrounded", &CharacterControllerComponent::IsGrounded,
			"GroundEntity", &CharacterControllerComponent::GroundEntity,
			"Move", &CharacterControllerComponent::Move,
			"Jump", &CharacterControllerComponent::Jump
		);
	}
}