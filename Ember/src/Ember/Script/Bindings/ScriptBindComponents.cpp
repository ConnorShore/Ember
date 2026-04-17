#include "ebpch.h"
#include "ScriptBindComponents.h"

#include "Ember/ECS/Component/Components.h"

namespace Ember {

	void BindComponents(sol::state& state)
	{
		state.new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale,
			"GetForward", &TransformComponent::GetForward
		);

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

		state.new_usertype<AnimatorComponent>("AnimatorComponent",
			"CurrentAnimationHandle", &AnimatorComponent::CurrentAnimationHandle,
			"CurrentTime", sol::property(
				[](AnimatorComponent& c) { return (float)c.CurrentTime; },
				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }
			),
			"PlaybackSpeed", &AnimatorComponent::PlaybackSpeed,
			"IsPlaying", &AnimatorComponent::IsPlaying,
			"Loop", &AnimatorComponent::Loop,
			"Crossfade", [](AnimatorComponent& c, UUID targetAnim, float duration) {
				if (c.CurrentAnimationHandle == targetAnim || targetAnim == Constants::InvalidUUID)
					return;
				if (c.CurrentAnimationHandle == Constants::InvalidUUID)
				{
					c.CurrentAnimationHandle = targetAnim;
					c.CurrentTime = 0.0f;
					c.IsPlaying = true;
					return;
				}

				c.PreviousAnimationHandle = c.CurrentAnimationHandle;
				c.PreviousTime = c.CurrentTime;
				c.CurrentAnimationHandle = targetAnim;
				c.CurrentTime = 0.0f;
				c.BlendDuration = duration;
				c.CurrentBlendTime = 0.0f;
				c.IsPlaying = true;
			}
		);

		state.new_usertype<CharacterControllerComponent>("CharacterControllerComponent",
			"WalkSpeed", &CharacterControllerComponent::WalkSpeed,
			"JumpForce", &CharacterControllerComponent::JumpForce,
			"GravityMultiplier", &CharacterControllerComponent::GravityMultiplier,
			"MaxSlopeAngle", &CharacterControllerComponent::MaxSlopeAngle,
			"MaxStepHeight", &CharacterControllerComponent::MaxStepHeight,

			"Move", &CharacterControllerComponent::Move,
			"Jump", &CharacterControllerComponent::Jump
		);
	}

}
