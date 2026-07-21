#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {
	void BindPhysicsComponents(sol::state& state)
	{
		// Bound as a resolving handle (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<RigidBodyComponent>>("RigidBodyComponent",
			"Mass", RefProp(&RigidBodyComponent::Mass),
			"GravityEnabled", RefProp(&RigidBodyComponent::GravityEnabled),
			"CurrentVelocity", RefMethod(&RigidBodyComponent::GetCurrentVelocity),
			"ApplyForce", RefMethod(&RigidBodyComponent::ApplyForce),
			"ApplyImpulse", RefMethod(&RigidBodyComponent::ApplyImpulse),
			"ApplyImpulseAtPoint", RefMethod(&RigidBodyComponent::ApplyImpulseAtPoint)
		);

		state.new_usertype<ColliderOffset>("ColliderOffset",
			"Position", &ColliderOffset::Position,
			"Rotation", &ColliderOffset::Rotation
		);

		state.new_usertype<ComponentRef<BoxColliderComponent>>("BoxColliderComponent",
			"Size", RefProp(&BoxColliderComponent::Size),
			"Offset", RefProp(&BoxColliderComponent::Offset),
			"Category", RefProp(&BoxColliderComponent::Category),
			"CollisionMask", RefProp(&BoxColliderComponent::CollisionMask)
		);

		state.new_usertype<ComponentRef<SphereColliderComponent>>("SphereColliderComponent",
			"Radius", RefProp(&SphereColliderComponent::Radius),
			"Offset", RefProp(&SphereColliderComponent::Offset),
			"Category", RefProp(&SphereColliderComponent::Category),
			"CollisionMask", RefProp(&SphereColliderComponent::CollisionMask)
		);

		state.new_usertype<ComponentRef<CapsuleColliderComponent>>("CapsuleColliderComponent",
			"Radius", RefProp(&CapsuleColliderComponent::Radius),
			"Height", RefProp(&CapsuleColliderComponent::Height),
			"Offset", RefProp(&CapsuleColliderComponent::Offset),
			"Category", RefProp(&CapsuleColliderComponent::Category),
			"CollisionMask", RefProp(&CapsuleColliderComponent::CollisionMask)
		);

		state.new_usertype<ComponentRef<ConvexMeshColliderComponent>>("ConvexMeshColliderComponent",
			"MeshHandle", RefProp(&ConvexMeshColliderComponent::MeshHandle),
			"Offset", RefProp(&ConvexMeshColliderComponent::Offset),
			"Category", RefProp(&ConvexMeshColliderComponent::Category),
			"CollisionMask", RefProp(&ConvexMeshColliderComponent::CollisionMask)
		);

		state.new_usertype<ComponentRef<ConcaveMeshColliderComponent>>("ConcaveMeshColliderComponent",
			"MeshHandle", RefProp(&ConcaveMeshColliderComponent::MeshHandle),
			"Offset", RefProp(&ConcaveMeshColliderComponent::Offset),
			"Category", RefProp(&ConcaveMeshColliderComponent::Category),
			"CollisionMask", RefProp(&ConcaveMeshColliderComponent::CollisionMask)
		);

		// Bound as a resolving handle (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<CharacterControllerComponent>>("CharacterControllerComponent",
			"WalkSpeed", RefProp(&CharacterControllerComponent::WalkSpeed),
			"JumpForce", RefProp(&CharacterControllerComponent::JumpForce),
			"Velocity", RefProp(&CharacterControllerComponent::Velocity),
			"RequestedMovement", RefProp(&CharacterControllerComponent::RequestedMovement),
			"GravityMultiplier", RefProp(&CharacterControllerComponent::GravityMultiplier),
			"MaxSlopeAngle", RefProp(&CharacterControllerComponent::MaxSlopeAngle),
			"MaxStepHeight", RefProp(&CharacterControllerComponent::MaxStepHeight),
			"IsGrounded", RefProp(&CharacterControllerComponent::IsGrounded),
			"GroundEntity", RefProp(&CharacterControllerComponent::GroundEntity),
			"MovementVelocity", RefProp(&CharacterControllerComponent::MovementVelocity),
			"Move", RefMethod(&CharacterControllerComponent::Move),
			"Jump", RefMethod(&CharacterControllerComponent::Jump)
		);
	}
}