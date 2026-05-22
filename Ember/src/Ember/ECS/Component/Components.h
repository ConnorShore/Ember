#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/Render/VFX/VFXTypes.h"
#include "Ember/ECS/Types.h"
#include "Ember/Core/Constants.h"
#include "Ember/Core/Application.h"
#include "Ember/Core/Filter.h"
#include "Ember/Physics/ColliderUserData.h"
#include "Ember/Asset/PhysicsMaterial.h"
#include "Ember/Audio/AudioSource.h"
#include "Ember/Audio/AudioSoundProperties.h"
#include "Ember/AI/NavNode.h"

#include <sol/sol.hpp>

#include <memory>
#include <string>
#include <functional>

#include <reactphysics3d/mathematics/mathematics.h>
#include <reactphysics3d/body/RigidBody.h>
#include <reactphysics3d/collision/shapes/BoxShape.h>
#include <reactphysics3d/collision/shapes/SphereShape.h>
#include <reactphysics3d/collision/shapes/CapsuleShape.h>
#include <reactphysics3d/collision/shapes/ConvexMeshShape.h>
#include <reactphysics3d/collision/shapes/ConcaveMeshShape.h>
#include <reactphysics3d/collision/VertexArray.h>
#include <reactphysics3d/collision/TriangleVertexArray.h>
#include <reactphysics3d/collision/TriangleMesh.h>

namespace Ember {

	struct DisabledComponent
	{
		DisabledComponent() = default;
		DisabledComponent(const DisabledComponent&) = default;
	};

	struct ColliderOffset
	{
		Vector3f Position = { 0.0f, 0.0f, 0.0f };
		Vector3f Rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles
	};

	struct IDComponent
	{
		UUID ID;
		IDComponent() : ID(UUID()) {}
		IDComponent(const UUID& id) : ID(id) {}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const std::string& tag) : Tag(tag) {}
		TagComponent(const TagComponent&) = default;
	};

	struct RelationshipComponent
	{
		UUID ParentHandle = Constants::InvalidUUID;
		std::vector<UUID> Children;

		bool IsAttachment = false; // If true, this entity will ignore the parent's scale

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;
	};

	struct TransformComponent
	{
		Vector3f Position;
		Vector3f Rotation;
		Vector3f Scale;

		Matrix4f WorldTransform = Matrix4f(1.0f);

		TransformComponent(const Vector3f& position = Vector3f(0.0f),
			const Vector3f& rotation = Vector3f(0.0f),
			const Vector3f& scale = Vector3f(1.0f))
			: Position(position), Rotation(rotation), Scale(scale) {
		}
		TransformComponent(const TransformComponent&) = default;

		Matrix4f GetLocalTransform() const
		{
			return Math::Translate(Position) * Math::GetRotationMatrix(Rotation) * Math::Scale(Scale);
		}

		const Matrix4f& GetWorldTransform() const
		{
			return WorldTransform;
		}

		Vector3f GetWorldPosition() const
		{
			return Vector3f(WorldTransform[3]);
		}

		// Extract basis vectors from the world transform matrix columns
		Vector3f GetForward() const
		{
			return Math::Normalize(Vector3f(
				-WorldTransform[2][0],
				-WorldTransform[2][1],
				-WorldTransform[2][2]
			));
		}

		Vector3f GetRight() const
		{
			return Math::Normalize(Vector3f(
				WorldTransform[0][0],
				WorldTransform[0][1],
				WorldTransform[0][2]
			));
		}

		Vector3f GetUp() const
		{
			return Math::Normalize(Vector3f(
				WorldTransform[1][0],
				WorldTransform[1][1],
				WorldTransform[1][2]
			));
		}
	};

	struct RigidBodyComponent
	{
		enum class BodyType { Static, Dynamic, Kinematic } Type = BodyType::Static;
		float Mass = 1.0f;
		bool GravityEnabled = true;

		Vector3f GetCurrentVelocity() const
		{
			if (Body)
			{
				reactphysics3d::Vector3 velocity = Body->getLinearVelocity();
				return Vector3f(velocity.x, velocity.y, velocity.z);
			}
			return Vector3f(0.0f);
		}

		void ApplyForce(const Vector3f& force)
		{
			if (Body)
			{
				Body->setIsSleeping(false);
				Body->applyWorldForceAtCenterOfMass(reactphysics3d::Vector3(force.x, force.y, force.z));
			}
		}

		void ApplyImpulse(const Vector3f& impulse)
		{
			if (Body)
			{
				Body->setIsSleeping(false);

				float mass = Body->getMass();
				if (mass > 0.0f)
				{
					// DeltaVelocity = Impulse / Mass
					reactphysics3d::Vector3 currentVelocity = Body->getLinearVelocity();
					reactphysics3d::Vector3 deltaVelocity(impulse.x / mass, impulse.y / mass, impulse.z / mass);

					// Apply the sudden burst of speed
					Body->setLinearVelocity(currentVelocity + deltaVelocity);
				}
			}
		}

		void ApplyImpulseAtPoint(const Vector3f& impulse, const Vector3f& worldPoint)
		{
			if (Body)
			{
				Body->setIsSleeping(false);

				float mass = Body->getMass();
				if (mass > 0.0f)
				{
					reactphysics3d::Vector3 rp3dImpulse(impulse.x, impulse.y, impulse.z);
					reactphysics3d::Vector3 rp3dPoint(worldPoint.x, worldPoint.y, worldPoint.z);

					// Accurately calculate the true center of mass in world space
					reactphysics3d::Vector3 worldCenterOfMass = Body->getTransform() * Body->getLocalCenterOfMass();

					// LINEAR VELOCITY (The Push)
					reactphysics3d::Vector3 deltaLinearVel = rp3dImpulse / mass;
					Body->setLinearVelocity(Body->getLinearVelocity() + deltaLinearVel);

					// ANGULAR VELOCITY (The Twist/Torque)
					reactphysics3d::Vector3 r = rp3dPoint - worldCenterOfMass;
					reactphysics3d::Vector3 angularImpulse = r.cross(rp3dImpulse);

					// RP3D stores the inertia tensor as a Vector3. 
					// We calculate the inverse by taking the reciprocal of each component safely.
					reactphysics3d::Vector3 localInertia = Body->getLocalInertiaTensor();
					reactphysics3d::Vector3 localInvI(
						localInertia.x != 0.0f ? 1.0f / localInertia.x : 0.0f,
						localInertia.y != 0.0f ? 1.0f / localInertia.y : 0.0f,
						localInertia.z != 0.0f ? 1.0f / localInertia.z : 0.0f
					);

					// Construct the diagonal 3x3 inverse matrix
					reactphysics3d::Matrix3x3 localInvInertia(
						localInvI.x, 0, 0,
						0, localInvI.y, 0,
						0, 0, localInvI.z
					);

					// Convert local inertia to world space to figure out how the object should spin
					reactphysics3d::Matrix3x3 rot = Body->getTransform().getOrientation().getMatrix();
					reactphysics3d::Matrix3x3 worldInvInertia = rot * localInvInertia * rot.getTranspose();

					// Apply the final rotation
					reactphysics3d::Vector3 deltaAngularVel = worldInvInertia * angularImpulse;
					Body->setAngularVelocity(Body->getAngularVelocity() + deltaAngularVel);
				}
			}
		}

		// Runtime only (not serialized) -> holds the actual physics body created in the PhysicsSystem
		reactphysics3d::RigidBody* Body = nullptr;

		RigidBodyComponent() = default;
		RigidBodyComponent(BodyType type, float mass = 1.0f, bool gravityEnabled = true) 
			: Type(type), Mass(mass), GravityEnabled(gravityEnabled) {}
		RigidBodyComponent(const RigidBodyComponent&) = default;
	};

	struct BoxColliderComponent
	{
		Vector3f Size = Vector3f(1.0f);
		ColliderOffset Offset;

		bool IsTrigger = false;
		bool PreviewCollider = false;

		Filter Category = FilterPreset::Default;
		Filter CollisionMask = FilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::BoxShape* Shape = nullptr;     // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)
		bool NeedsRebuild = false;
		ColliderUserData UserData;
		Vector3f CachedWorldScale = Vector3f(0.0f);

		BoxColliderComponent() = default;
		BoxColliderComponent(const Vector3f& size)
			: Size(size) {}
		BoxColliderComponent(const BoxColliderComponent&) = default;
	};

	struct SphereColliderComponent
	{
		float Radius = 0.5f;
		ColliderOffset Offset;

		bool IsTrigger = false;
		bool PreviewCollider = false;

		Filter Category = FilterPreset::Default;
		Filter CollisionMask = FilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::SphereShape* Shape = nullptr; // The raw geometry
		reactphysics3d::Collider* Collider = nullptr; // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)
		bool NeedsRebuild = false;
		ColliderUserData UserData;
		Vector3f CachedWorldScale = Vector3f(0.0f);

		SphereColliderComponent() = default;
		SphereColliderComponent(float radius)
			: Radius(radius) {}
		SphereColliderComponent(const SphereColliderComponent&) = default;
	};

	struct CapsuleColliderComponent
	{
		float Radius = 0.5f;
		float Height = 2.0f;
		ColliderOffset Offset;

		bool IsTrigger = false;
		bool PreviewCollider = false;

		Filter Category = FilterPreset::Default;
		Filter CollisionMask = FilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::CapsuleShape* Shape = nullptr; // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr;  // The body this collider is attached to (cached for easy access)
		bool NeedsRebuild = false;
		ColliderUserData UserData;
		Vector3f CachedWorldScale = Vector3f(0.0f);

		CapsuleColliderComponent() = default;
		CapsuleColliderComponent(float radius, float height)
			: Radius(radius), Height(height) {
		}
		CapsuleColliderComponent(const CapsuleColliderComponent&) = default;
	};

	struct ConvexMeshColliderComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;
		ColliderOffset Offset;

		bool IsTrigger = false;
		bool PreviewCollider = false;

		Filter Category = FilterPreset::Default;
		Filter CollisionMask = FilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::ConvexMeshShape* Shape = nullptr;   // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;		// The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr;		// The body this collider is attached to (cached for easy access)
		bool NeedsRebuild = false;
		ColliderUserData UserData;
		Vector3f CachedWorldScale = Vector3f(0.0f);

		std::vector<float> PhysicsVertices;
		reactphysics3d::VertexArray* RP3DVertexArray = nullptr;

		ConvexMeshColliderComponent() = default;
		ConvexMeshColliderComponent(UUID meshHandle)
			: MeshHandle(meshHandle) {
		}
		ConvexMeshColliderComponent(const ConvexMeshColliderComponent&) = default;
	};

	struct ConcaveMeshColliderComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;
		ColliderOffset Offset;

		bool IsTrigger = false;
		bool PreviewCollider = false;

		Filter Category = FilterPreset::Default;
		Filter CollisionMask = FilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::ConcaveMeshShape* Shape = nullptr;  // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;		// The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr;		// The body this collider is attached to (cached for easy access)
		bool NeedsRebuild = false;
		ColliderUserData UserData;
		Vector3f CachedWorldScale = Vector3f(0.0f);

		std::vector<float> PhysicsVertices;
		std::vector<uint32_t> PhysicsIndices;
		reactphysics3d::TriangleVertexArray* TriangleArray = nullptr;
		reactphysics3d::TriangleMesh* TriangleMesh = nullptr;

		ConcaveMeshColliderComponent() = default;
		ConcaveMeshColliderComponent(UUID meshHandle)
			: MeshHandle(meshHandle) {
		}
		ConcaveMeshColliderComponent(const ConcaveMeshColliderComponent&) = default;
	};

	struct CharacterControllerComponent
	{
		// User defined properties
		float WalkSpeed = 5.0f;
		float JumpForce = 5.0f;
		float GravityMultiplier = 1.0f;
		float MaxSlopeAngle = 45.0f;
		float MaxStepHeight = 0.25f;

		// Read-only properties for Lua scripts (not serialized)
		bool IsGrounded = false;
		EntityID GroundEntity = Constants::Entities::InvalidEntityID;
		Vector3f Velocity = Vector3f(0.0f);
		Vector3f RequestedMovement = Vector3f(0.0f);
		// Combined movement velocity (input + physics) in world units/second, updated each frame
		Vector3f MovementVelocity = Vector3f(0.0f);

		void Move(const Vector3f& requestedMovement)
		{
			RequestedMovement = requestedMovement;
		}

		void Jump()
		{
			if (IsGrounded)
				Velocity.y = JumpForce;
		}

		CharacterControllerComponent() = default;
		CharacterControllerComponent(const CharacterControllerComponent&) = default;
	};

	struct SpriteComponent
	{
		Vector4f Color = Vector4f(1.0f);
		UUID TextureHandle = Constants::InvalidUUID;
		bool ScreenSpace = true;

		SpriteComponent() = default;
		SpriteComponent(const Vector4f color) : Color(color) {}
		SpriteComponent(UUID texId) : TextureHandle(texId) {}
		SpriteComponent(const Vector4f color, UUID texId) : Color(color), TextureHandle(texId) {}
	};

	struct StaticMeshComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;
		Filter Layer = FilterPreset::Default;

		StaticMeshComponent() = default;
		StaticMeshComponent(UUID meshId) : MeshHandle(meshId) {}
		StaticMeshComponent(const StaticMeshComponent&) = default;
	};

	struct SkinnedMeshComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;
		UUID AnimatorEntityHandle = Constants::InvalidUUID;
		Filter Layer = FilterPreset::Default;

		// Runtime only (not serialized) -> used for caching animator id to avoid expensive lookups
		EntityID RuntimeAnimatorID = Constants::Entities::InvalidEntityID;


		SkinnedMeshComponent() = default;
		SkinnedMeshComponent(UUID meshId, UUID animatorEntityUUID = Constants::InvalidUUID)
			: MeshHandle(meshId), AnimatorEntityHandle(animatorEntityUUID) {
		}
		SkinnedMeshComponent(const SkinnedMeshComponent&) = default;
	};

	struct MaterialComponent
	{
		UUID MaterialHandle = Constants::InvalidUUID;

		MaterialComponent() = default;
		MaterialComponent(UUID handle) : MaterialHandle(handle) {}
		MaterialComponent(const MaterialComponent&) = default;

		// Lazily creates a MaterialInstance from a base Material so this entity
		// gets its own copy of uniforms without affecting other users of the same material
		SharedPtr<MaterialInstance> GetInstanced(const std::string& materialInstanceName)
		{
			if (MaterialHandle == Constants::InvalidUUID)
				return nullptr;

			auto& assetManager = Application::Instance().GetAssetManager();
			auto materialAsset = assetManager.GetAsset<MaterialBase>(MaterialHandle);

			if (!materialAsset)
				return nullptr;

			// If already an instance, just return it
			if (DynamicPointerCast<MaterialInstance>(materialAsset) != nullptr)
				return DynamicPointerCast<MaterialInstance>(materialAsset);

			if (DynamicPointerCast<Ember::Material>(materialAsset) != nullptr)
			{
				auto base = DynamicPointerCast<Material>(materialAsset);
				auto newInstance = SharedPtr<MaterialInstance>::Create(materialInstanceName, base);
				newInstance->SetIsEngineAsset(false);
				// TODO: Find way to avoid name clashing if multiple instances of the same material are created with the same name
				assetManager.Register(newInstance);

				MaterialHandle = newInstance->GetUUID();
				return newInstance;
			}

			EB_CORE_ASSERT(false, "Unknown Material type!");
			return nullptr;
		}

		SharedPtr<MaterialInstance> CloneMaterial(const std::string& newName)
		{
			if (MaterialHandle == Constants::InvalidUUID)
				return nullptr;

			auto& assetManager = Application::Instance().GetAssetManager();
			auto materialAsset = assetManager.GetAsset<MaterialBase>(MaterialHandle);

			SharedPtr<MaterialInstance> newInstance = nullptr;

			if (auto base = DynamicPointerCast<Material>(materialAsset))
			{
				newInstance = SharedPtr<MaterialInstance>::Create(newName, base);
			}
			else if (auto instance = DynamicPointerCast<MaterialInstance>(materialAsset))
			{
				auto baseMaterial = instance->GetMaterial();
				newInstance = SharedPtr<MaterialInstance>::Create(newName, baseMaterial);

				// Copy uniforms over
				for (const auto& [uniformName, value] : instance->GetUniforms())
					newInstance->SetUniform(uniformName, value);
			}
			else
			{
				EB_CORE_ASSERT(false, "Unknown Material type!");
				return nullptr;
			}

			newInstance->SetIsEngineAsset(false);
			assetManager.Register(newInstance);

			MaterialHandle = newInstance->GetUUID();

			return newInstance;
		}
	};

	struct CameraComponent
	{
		Camera Camera;
		bool IsActive = false;
		Filter RenderMask = FilterPreset::All;
		Filter VolumeMask = FilterPreset::All;

		CameraComponent() = default;
		CameraComponent(const Ember::Camera& camera, bool active = false) : Camera(camera), IsActive(active) {}
		CameraComponent(const CameraComponent&) = default;
	};

	struct DirectionalLightComponent
	{
		bool Active = true;
		Vector3f Color = Vector3f(1.0f);
		float Intensity = 5.0f;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const Vector3f& color, float intensity)
			: Color(color), Intensity(intensity) { }
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
	};

	struct SpotLightComponent
	{
		bool Active = true;
		Vector3f Color = Vector3f(1.0f);
		float Intensity = 100.0f;

		// Store angles in radians for the C++ side (camera frustum, UI)
		float CutOffAngle = Math::Radians(12.5f);
		float OuterCutOffAngle = Math::Radians(17.5f);

		// Store as cosine values for direct use in GLSL (avoids per-fragment acos)
		float CutOff = cos(Math::Radians(12.5f));
		float OuterCutOff = cos(Math::Radians(17.5f));;

		SpotLightComponent() = default;
		SpotLightComponent(const Vector3f& color, float intensity, float cutOffDeg, float outerCutOffDeg)
			: Color(color), Intensity(intensity),
			CutOffAngle(Math::Radians(cutOffDeg)),
			CutOff(cos(Math::Radians(cutOffDeg))),
			OuterCutOffAngle(Math::Radians(outerCutOffDeg)),
			OuterCutOff(cos(Math::Radians(outerCutOffDeg)))
		{
		}
		SpotLightComponent(const SpotLightComponent&) = default;
	};

	struct PointLightComponent
	{
		bool Active = true;
		Vector3f Color = Vector3f(1.0f);
		float Intensity = 25.0f;
		float Radius = 0.0f;

		PointLightComponent() = default;
		PointLightComponent(const Vector3f& color, float intensity, float radius)
			: Color(color), Intensity(intensity), Radius(radius) { }
		PointLightComponent(const PointLightComponent&) = default;
	};

	struct ScriptComponent
	{
		UUID ScriptHandle = Constants::InvalidUUID;

		sol::table Instance;
		bool Initialized = false;

		// Cache user property overrides
		std::unordered_map<std::string, ScriptProperty> UserPropertyOverrides;

		ScriptComponent() = default;
		ScriptComponent(UUID scriptUUID) : ScriptHandle(scriptUUID) {}
		ScriptComponent(const ScriptComponent&) = default;
	};

	struct OutlineComponent
	{
		Vector3f Color = Vector3f(1.0f);
		float Thickness = 1.0f;

		OutlineComponent() = default;
		OutlineComponent(const Vector3f& color, float thickness)
			: Color(color), Thickness(thickness) {}
		OutlineComponent(const OutlineComponent&) = default;
	};

	struct BillboardComponent
	{
		UUID TextureHandle = Constants::Assets::DefaultWhiteTexUUID;
		Vector4f Tint = Vector4f(1.0f);
		bool Spherical = true;
		bool StaticSize = true;
		bool RenderRuntime = false;
		float Size = 1.0f;

		BillboardComponent() = default;
		BillboardComponent(const BillboardComponent&) = default;
	};

	struct AnimatorComponent
	{
		UUID SkeletonHandle = Constants::InvalidUUID;
		UUID CurrentAnimationHandle = Constants::InvalidUUID;

		// Runtime data
		TimeStep CurrentTime = 0.0f;
		TimeStep PreviousTime = 0.0f;
		float PlaybackSpeed = 1.0f;
		bool IsPlaying = true;
		bool Loop = true;

		// Blending
		UUID PreviousAnimationHandle = Constants::InvalidUUID;
		float BlendDuration = 0.0f;
		float CurrentBlendTime = 0.0f;

		// Caches
		std::vector<Matrix4f> BonePoseMatrices = std::vector<Matrix4f>(Constants::Renderer::MaxBones, Matrix4f(1.0f));
		std::vector<Matrix4f> BoneMatrices = std::vector<Matrix4f>(Constants::Renderer::MaxBones, Matrix4f(1.0f));

		void PlayAnimation(const std::string& name, float playbackSpeed = 1.0f, float blendDuration = 0.0f)
		{
			PlaybackSpeed = playbackSpeed;
			Loop = false;
			CrossfadeToAnimation(name, blendDuration);
		}

		void PlayLoopAnimation(const std::string& name, float playbackSpeed = 1.0f, float blendDuration = 0.0f)
		{
			PlaybackSpeed = playbackSpeed;
			Loop = true;
			CrossfadeToAnimation(name, blendDuration);
		}

		void CrossfadeToAnimation(const std::string& name, float blendDuration)
		{
			auto& assetManager = Application::Instance().GetAssetManager();
			auto animationAsset = assetManager.GetAsset<Animation>(name);
			UUID targetAnim = animationAsset ? animationAsset->GetUUID() : (UUID)Constants::InvalidUUID;
			if (CurrentAnimationHandle == targetAnim || targetAnim == Constants::InvalidUUID)
				return;

			CurrentAnimationHandle = targetAnim;
			if (CurrentAnimationHandle == Constants::InvalidUUID)
			{
				CurrentAnimationHandle = targetAnim;
				CurrentTime = 0.0f;
				IsPlaying = true;
				return;
			}

			PreviousAnimationHandle = CurrentAnimationHandle;
			PreviousTime = CurrentTime;
			CurrentAnimationHandle = targetAnim;
			CurrentTime = 0.0f;
			BlendDuration = blendDuration;
			CurrentBlendTime = 0.0f;
			IsPlaying = true;
		}

		AnimatorComponent() = default;
		AnimatorComponent(UUID skeletonUUID, UUID animationUUID)
			: SkeletonHandle(skeletonUUID), CurrentAnimationHandle(animationUUID) {}
		AnimatorComponent(const AnimatorComponent&) = default;
	};

	struct BoneSocketComponent
	{
		UUID TargetEntityHandle = Constants::InvalidUUID;
		std::string BoneName;

		Vector3f Position = Vector3f(0.0f);
		Vector3f Rotation = Vector3f(0.0f);
		Vector3f Scale = Vector3f(1.0f);

		int32_t RuntimeBoneIndex = -1;
		std::string RuntimeBoneName;

		BoneSocketComponent() = default;
		BoneSocketComponent(UUID targetEntityHandle, const std::string& boneName)
			: TargetEntityHandle(targetEntityHandle), BoneName(boneName) {}
		BoneSocketComponent(const BoneSocketComponent&) = default;

		Matrix4f GetLocalTransform() const
		{
			return Math::Translate(Position) * Math::GetRotationMatrix(Rotation) * Math::Scale(Scale);
		}
	};

	struct PrefabComponent 
	{
		UUID PrefabHandle = Constants::InvalidUUID;

		PrefabComponent() = default;
		PrefabComponent(UUID prefabUUID) : PrefabHandle(prefabUUID) {}
		PrefabComponent(const PrefabComponent&) = default;
	};

	struct LifetimeComponent
	{
		float Lifetime = 0.0f;        // Remaining lifetime in seconds
		float InitialLifetime = 0.0f; // Original lifetime; used to reset when returned to a pool

		LifetimeComponent() = default;
		LifetimeComponent(float lifetime) : Lifetime(lifetime), InitialLifetime(lifetime) {}
		LifetimeComponent(const LifetimeComponent&) = default;
	};

	struct TextComponent
	{
		std::string Text;
		Vector4f Color = Vector4f(1.0f);
		UUID FontHandle = Constants::InvalidUUID;
		bool ScreenSpace = true;

		TextComponent() = default;
		TextComponent(const std::string& text, const Vector4f& color, UUID fontHandle, bool screenSpace)
			: Text(text), Color(color), FontHandle(fontHandle), ScreenSpace(screenSpace) {}
		TextComponent(const TextComponent&) = default;
	};

	struct PoolComponent
	{
		std::string PoolID;	// i.e. "Bullet"

		PoolComponent() = default;
		PoolComponent(const std::string& id) : PoolID(id) {}
		PoolComponent(const PoolComponent&) = default;
	};

	struct PoolConfigComponent
	{
		std::string PoolID;
		uint32_t Capacity = 0;
		bool LoopEntities = true;
		UUID PrefabHandle = Constants::InvalidUUID;

		PoolConfigComponent() = default;
		PoolConfigComponent(const std::string& id, uint32_t capacity, UUID prefabHandle)
			: PoolID(id), Capacity(capacity), PrefabHandle(prefabHandle) {
		}
		PoolConfigComponent(const PoolConfigComponent&) = default;
	};


	struct ParticleEmitterComponent
	{
		// How many particles to spawn per second
		float EmissionRate = 50.0f;
		float EmissionAccumulator = 0.0f; // Runtime only: Accumulates time to determine when to emit particles

		float GravityMultiplier = 0.0f; // How much gravity affects the particles (0 = no gravity, 1 = normal gravity, -1 = floats up)

		// Physics
		Vector3f Velocity = { 0.0f, 1.0f, 0.0f };
		Vector3f VelocityVariation = { 0.5f, 0.1f, 0.5f };

		float Drag = 0.1f; // How much air resistance slows down particles over time
		float AngularVelocity = 0.0f; // How fast particles rotate
		float AngularVelocityVariation = 0.2f;

		bool AlignWithVelocity = false; // If true, particles will rotate to face the direction they're moving
		float StretchFactor = 0.0f; // If AlignWithVelocity is true, this controls how much particles stretch in the direction of movement (0 = no stretch, 1 = full stretch)

		// Visuals (Interpolated over lifetime)
		Vector4f ColorBegin = { 1.0f, 0.0f, 0.0f, 1.0f };
		Vector4f ColorEnd = { 0.0f, 0.0f, 1.0f, 0.0f }; // Fades to blue and transparent

		float ScaleBegin = 1.0f;
		float ScaleEnd = 0.1f;
		float ScaleVariation = 0.3f;

		UUID TextureHandle = Constants::Assets::DefaultWhiteTexUUID;

		float Lifetime = 1.0f;
		float LifetimeVariation = 0.2f;

		bool IsActive = true;

		ParticleEmitterComponent() = default;
		ParticleEmitterComponent(const ParticleEmitterComponent&) = default;
	};

	struct PostProcessVolumeComponent
	{
		PostProcessVolumeSettings Settings;
		uint32_t Priority;

		float BlendRadius = 1.0f;

		PostProcessVolumeComponent() = default;
		PostProcessVolumeComponent(const PostProcessVolumeComponent&) = default;
	};

	struct AudioSourceComponent
	{
		AudioSource Source;
		AudioSoundProperties Properties;
		UUID AudioClipHandle = Constants::InvalidUUID;

		AudioSourceComponent() = default;

		// Ban copies
		AudioSourceComponent(const AudioSourceComponent&) = delete;
		AudioSourceComponent& operator=(const AudioSourceComponent&) = delete;

		// Use default move semantics
		AudioSourceComponent(AudioSourceComponent&&) = default;
		AudioSourceComponent& operator=(AudioSourceComponent&&) = default;
	};

	struct SingleSoundComponent
	{
		// Leave empty
	};

	struct AudioListenerComponent
	{
		uint32_t ListenerIndex = 0;
		bool IsActive = true;

		AudioListenerComponent() = default;
		AudioListenerComponent(const AudioListenerComponent&) = default;
	};

	struct WaypointComponent
	{
		// Editor only prop to show all connected paths when waypoint entity is selected
		bool ShowPaths = true;

		WaypointComponent() = default;
		WaypointComponent(const WaypointComponent&) = default;
	};

	struct AIAgentComponent
	{
		enum class PathMode { Manual, Dynamic };
		PathMode Mode = PathMode::Manual;

		// Manual properties
		std::vector<UUID> ManualWaypoints;
		bool Loop = true;

		// Dynamic properties
		UUID TargetEntity = Constants::InvalidUUID;
		UUID GridEntity = Constants::InvalidUUID;
		float RecalculateInterval = 1.0f;

		// Runtime only (not serialized)
		float TimeSinceLastRecalculate = 0.0f;
		bool Dirty = false; // Set to true when the agent needs to update its mode

		AIAgentComponent() = default;
		AIAgentComponent(const AIAgentComponent&) = default;
	};

	struct AIPathComponent
	{
		std::vector<Vector3f> Waypoints;	// Positions to navigate to
		float Speed = 1.0f;
		bool Loop = true;
		float ArrivalTolerance = 0.1f; // How close to a waypoint before we consider it "reached"

		// Runtime only (not serialized)
		uint32_t CurrentWaypointIndex = 0;

		AIPathComponent() = default;
		AIPathComponent(const AIPathComponent&) = default;
	};

	struct NavigationGridComponent
	{  
		float NodeSpacing = 1.0f;   // Size of each grid square
		bool Generated = false;    // Has the grid been generated yet (used to trigger generation in the NavigationSystem)

		// TODO: Maybe make nav nodes to shared ptrs or something to avoid copying them around so much
		std::vector<std::vector<NavNode>> Grid;

		NavigationGridComponent() = default;
		NavigationGridComponent(const NavigationGridComponent&) = default;
	};

	struct LocalAvoidanceComponent
	{
		float AvoidanceRadius = 0.5f; // How close other agents can get before we start avoiding them
		float AvoidanceStrength = 1.0f; // How strongly we try to avoid other agents (0 = ignore, 1 = full avoidance)
		Filter AvoidanceMask = FilterPreset::Default; // Which agents to avoid (use collision filters to specify)

		// Runtime only (not serialized)
		Vector3f AvoidanceVector = Vector3f(0.0f); // The current offset being applied to avoid other agents

		LocalAvoidanceComponent() = default;
		LocalAvoidanceComponent(const LocalAvoidanceComponent&) = default;
	};

}