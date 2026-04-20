#include "ebpch.h"
#include "ScriptBindComponents.h"

namespace Ember {
	void BindAllComponents(sol::state& state)
	{
		BindCoreComponents(state);
		BindPhysicsComponents(state);
		BindRenderingComponents(state);
		BindLightingAndCameraComponents(state);
		BindMiscComponents(state);
	}
}

//#include "ebpch.h"
//#include "ScriptBindComponents.h"
//
//#include "Ember/ECS/Component/Components.h"
//
//namespace Ember {
//
//	void BindComponents(sol::state& state)
//	{
//		state.new_usertype<TransformComponent>("TransformComponent",
//			"Position", &TransformComponent::Position,
//			"Rotation", &TransformComponent::Rotation,
//			"Scale", &TransformComponent::Scale,
//			"WorldPosition", sol::property([](TransformComponent& c) { return Vector3f(c.GetWorldTransform()[3]); }),
//			"WorldRotation", sol::property([](TransformComponent& c) { return Math::ToEulerAngles(glm::quat_cast(c.GetWorldTransform())); }),
//			"GetForward", &TransformComponent::GetForward
//		);
//
//		state.new_usertype<RigidBodyComponent>("RigidBodyComponent",
//			"Mass", &RigidBodyComponent::Mass,
//			"GravityEnabled", &RigidBodyComponent::GravityEnabled,
//			"ApplyForce", &RigidBodyComponent::ApplyForce,
//			"ApplyImpulse", &RigidBodyComponent::ApplyImpulse
//		);
//
//		state.new_usertype<ColliderOffset>("ColliderOffset",
//			"Position", &ColliderOffset::Position,
//			"Rotation", &ColliderOffset::Rotation
//		);
//
//		state.new_usertype<BoxColliderComponent>("BoxColliderComponent",
//			"Size", &BoxColliderComponent::Size,
//			"Offset", &BoxColliderComponent::Offset,
//			"Category", &BoxColliderComponent::Category,
//			"CollisionMask", &BoxColliderComponent::CollisionMask
//		);
//
//		state.new_usertype<SphereColliderComponent>("SphereColliderComponent",
//			"Radius", &SphereColliderComponent::Radius,
//			"Offset", &SphereColliderComponent::Offset,
//			"Category", &SphereColliderComponent::Category,
//			"CollisionMask", &SphereColliderComponent::CollisionMask
//		);
//
//		state.new_usertype<CapsuleColliderComponent>("CapsuleColliderComponent",
//			"Radius", &CapsuleColliderComponent::Radius,
//			"Height", &CapsuleColliderComponent::Height,
//			"Offset", &CapsuleColliderComponent::Offset,
//			"Category", &CapsuleColliderComponent::Category,
//			"CollisionMask", &CapsuleColliderComponent::CollisionMask
//		);
//
//		state.new_usertype<ConvexMeshColliderComponent>("ConvexMeshColliderComponent",
//			"MeshHandle", &ConvexMeshColliderComponent::MeshHandle,
//			"Offset", &ConvexMeshColliderComponent::Offset,
//			"Category", &ConvexMeshColliderComponent::Category,
//			"CollisionMask", &ConvexMeshColliderComponent::CollisionMask
//		);
//
//		state.new_usertype<ConcaveMeshColliderComponent>("ConcaveMeshColliderComponent",
//			"MeshHandle", &ConcaveMeshColliderComponent::MeshHandle,
//			"Offset", &ConcaveMeshColliderComponent::Offset,
//			"Category", &ConcaveMeshColliderComponent::Category,
//			"CollisionMask", &ConcaveMeshColliderComponent::CollisionMask
//		);
//
//		state.new_usertype<CharacterControllerComponent>("CharacterControllerComponent",
//			"WalkSpeed", &CharacterControllerComponent::WalkSpeed,
//			"JumpForce", &CharacterControllerComponent::JumpForce,
//			"GravityMultiplier", &CharacterControllerComponent::GravityMultiplier,
//			"MaxSlopeAngle", &CharacterControllerComponent::MaxSlopeAngle,
//			"MaxStepHeight", &CharacterControllerComponent::MaxStepHeight,
//			"IsGrounded", &CharacterControllerComponent::IsGrounded,
//			"GroundEntity", &CharacterControllerComponent::GroundEntity,
//
//			"Move", &CharacterControllerComponent::Move,
//			"Jump", &CharacterControllerComponent::Jump
//		);
//
//		state.new_usertype<SpriteComponent>("SpriteRendererComponent",
//			"Color", &SpriteComponent::Color,
//			"TextureHandle", &SpriteComponent::TextureHandle
//		);
//
//		state.new_usertype<StaticMeshComponent>("StaticMeshComponent",
//			"MeshHandle", &StaticMeshComponent::MeshHandle
//		);
//
//		state.new_usertype<SkinnedMeshComponent>("SkinnedMeshComponent",
//			"MeshHandle", &SkinnedMeshComponent::MeshHandle,
//			"AnimatorEntityHandle", &SkinnedMeshComponent::AnimatorEntityHandle
//		);
//
//		state.new_usertype<MaterialComponent>("MaterialComponent",
//			"MaterialHandle", &MaterialComponent::MaterialHandle,
//			"GetInstanced", &MaterialComponent::GetInstanced,
//			"CloneMaterial", &MaterialComponent::CloneMaterial
//		);
//
//		state.new_usertype<CameraComponent>("CameraComponent",
//			"IsActive", &CameraComponent::IsActive,
//
//			// Projection Type
//			"ProjectionType", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetProjectionType(); },
//				[](CameraComponent& c, Camera::ProjectionType type) { c.Camera.SetProjectionType(type); }
//			),
//
//			// Perspective Properties
//			"FieldOfView", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().FieldOfView; },
//				[](CameraComponent& c, float fov) {
//					auto& props = c.Camera.GetPerspectiveProps();
//					// Use the existing SetPerspective method so the matrix recalculates!
//					c.Camera.SetPerspective(fov, props.NearClip, props.FarClip);
//				}
//			),
//			"PerspectiveNear", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().NearClip; },
//				[](CameraComponent& c, float nearClip) {
//					auto& props = c.Camera.GetPerspectiveProps();
//					c.Camera.SetPerspective(props.FieldOfView, nearClip, props.FarClip);
//				}
//			),
//			"PerspectiveFar", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().FarClip; },
//				[](CameraComponent& c, float farClip) {
//					auto& props = c.Camera.GetPerspectiveProps();
//					c.Camera.SetPerspective(props.FieldOfView, props.NearClip, farClip);
//				}
//			),
//
//			// Orthographic Properties
//			"OrthographicSize", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().Size; },
//				[](CameraComponent& c, float size) {
//					auto& props = c.Camera.GetOrthographicProps();
//					c.Camera.SetOrthographic(size, props.NearClip, props.FarClip);
//				}
//			),
//			"OrthographicNear", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().NearClip; },
//				[](CameraComponent& c, float nearClip) {
//					auto& props = c.Camera.GetOrthographicProps();
//					c.Camera.SetOrthographic(props.Size, nearClip, props.FarClip);
//				}
//			),
//			"OrthographicFar", sol::property(
//				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().FarClip; },
//				[](CameraComponent& c, float farClip) {
//					auto& props = c.Camera.GetOrthographicProps();
//					c.Camera.SetOrthographic(props.Size, props.NearClip, farClip);
//				}
//			)
//		);
//
//		state.new_usertype<DirectionalLightComponent>("DirectionalLightComponent",
//			"Color", &DirectionalLightComponent::Color,
//			"Intensity", &DirectionalLightComponent::Intensity
//		);
//
//		state.new_usertype<PointLightComponent>("PointLightComponent",
//			"Color", &PointLightComponent::Color,
//			"Intensity", &PointLightComponent::Intensity,
//			"Radius", &PointLightComponent::Radius
//		);
//
//		state.new_usertype<SpotLightComponent>("SpotLightComponent",
//			"Color", &SpotLightComponent::Color,
//			"Intensity", &SpotLightComponent::Intensity,
//			"CutOffAngle", &SpotLightComponent::CutOffAngle,
//			"OuterCutOffAngle", &SpotLightComponent::CutOffAngle
//		);
//
//		state.new_usertype<OutlineComponent>("OutlineComponent",
//			"Color", &OutlineComponent::Color,
//			"Thickness", &OutlineComponent::Thickness
//		);
//
//		state.new_usertype<BillboardComponent>("BillboardComponent",
//			"Tint", &BillboardComponent::Tint,
//			"TextureHandle", &BillboardComponent::TextureHandle,
//			"Size", &BillboardComponent::Size,
//			"IsSpherical", &BillboardComponent::Spherical,
//			"IsStaticSize", &BillboardComponent::StaticSize
//		);
//
//		state.new_usertype<AnimatorComponent>("AnimatorComponent",
//			"CurrentAnimationHandle", &AnimatorComponent::CurrentAnimationHandle,
//			"CurrentTime", sol::property(
//				[](AnimatorComponent& c) { return (float)c.CurrentTime; },
//				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }
//			),
//			"PlaybackSpeed", &AnimatorComponent::PlaybackSpeed,
//			"IsPlaying", &AnimatorComponent::IsPlaying,
//			"Loop", &AnimatorComponent::Loop,
//			"Crossfade", [](AnimatorComponent& c, UUID targetAnim, float duration) {
//				if (c.CurrentAnimationHandle == targetAnim || targetAnim == Constants::InvalidUUID)
//					return;
//				if (c.CurrentAnimationHandle == Constants::InvalidUUID)
//				{
//					c.CurrentAnimationHandle = targetAnim;
//					c.CurrentTime = 0.0f;
//					c.IsPlaying = true;
//					return;
//				}
//
//				c.PreviousAnimationHandle = c.CurrentAnimationHandle;
//				c.PreviousTime = c.CurrentTime;
//				c.CurrentAnimationHandle = targetAnim;
//				c.CurrentTime = 0.0f;
//				c.BlendDuration = duration;
//				c.CurrentBlendTime = 0.0f;
//				c.IsPlaying = true;
//			}
//		);
//
//		state.new_usertype<LifetimeComponent>("LifetimeComponent",
//			"Lifetime", &LifetimeComponent::Lifetime
//		);
//
//		state.new_usertype<TextComponent>("TextComponent",
//			"Text", &TextComponent::Text,
//			//"Font", //TODO: Make this take in a string and look up the asset handle to assign
//			"Color", &TextComponent::Color
//		);
//	}
//
//}
