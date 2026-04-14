#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture2D.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/ECS/Types.h"
#include "Ember/Core/Constants.h"
#include "Ember/Core/Application.h"
#include "Ember/Physics/CollisionFilter.h"
#include "Ember/Asset/PhysicsMaterial.h"

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
		Vector3f Offset = Vector3f(0.0f);

		CollisionFilter Category = CollisionFilterPreset::Default;
		CollisionFilter CollisionMask = CollisionFilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::BoxShape* Shape = nullptr;     // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)

		BoxColliderComponent() = default;
		BoxColliderComponent(const Vector3f& size, const Vector3f& offset = Vector3f(0.0f))
			: Size(size), Offset(offset) {}
		BoxColliderComponent(const BoxColliderComponent&) = default;
	};

	struct SphereColliderComponent
	{
		float Radius = 0.5f;
		Vector3f Offset = Vector3f(0.0f);

		CollisionFilter Category = CollisionFilterPreset::Default;
		CollisionFilter CollisionMask = CollisionFilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::SphereShape* Shape = nullptr;   // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)

		SphereColliderComponent() = default;
		SphereColliderComponent(float radius, const Vector3f& offset = Vector3f(0.0f))
			: Radius(radius), Offset(offset) {}
		SphereColliderComponent(const SphereColliderComponent&) = default;
	};

	struct CapsuleColliderComponent
	{
		float Radius = 0.5f;
		float Height = 2.0f;
		Vector3f Offset = Vector3f(0.0f);

		CollisionFilter Category = CollisionFilterPreset::Default;
		CollisionFilter CollisionMask = CollisionFilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::CapsuleShape* Shape = nullptr;   // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)

		CapsuleColliderComponent() = default;
		CapsuleColliderComponent(float radius, float height, const Vector3f& offset = Vector3f(0.0f))
			: Radius(radius), Height(height), Offset(offset) {
		}
		CapsuleColliderComponent(const CapsuleColliderComponent&) = default;
	};

	struct ConvexMeshColliderComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;

		CollisionFilter Category = CollisionFilterPreset::Default;
		CollisionFilter CollisionMask = CollisionFilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::ConvexMeshShape* Shape = nullptr;   // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)

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

		CollisionFilter Category = CollisionFilterPreset::Default;
		CollisionFilter CollisionMask = CollisionFilterPreset::Default;

		UUID PhysicsMaterialHandle = Constants::InvalidUUID;

		// Runtime only (not serialized) -> holds the actual collider created in the PhysicsSystem
		reactphysics3d::ConcaveMeshShape* Shape = nullptr;   // The raw geometry
		reactphysics3d::Collider* Collider = nullptr;  // The attachment to the body
		reactphysics3d::Body* AttachedBody = nullptr; // The body this collider is attached to (cached for easy access)

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

	struct SpriteComponent
	{
		Vector4f Color = Vector4f(1.0f);
		UUID TextureHandle = Constants::InvalidUUID;;

		SpriteComponent(const Vector4f color) : Color(color) {}
		SpriteComponent(UUID texId) : TextureHandle(texId) {}
		SpriteComponent(const Vector4f color, UUID texId) : Color(color), TextureHandle(texId) {}
	};

	struct StaticMeshComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;

		StaticMeshComponent() = default;
		StaticMeshComponent(UUID meshId) : MeshHandle(meshId) {}
		StaticMeshComponent(const StaticMeshComponent&) = default;
	};

	struct SkinnedMeshComponent
	{
		UUID MeshHandle = Constants::InvalidUUID;
		UUID AnimatorEntityHandle = Constants::InvalidUUID;

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

		CameraComponent() = default;
		CameraComponent(const Ember::Camera& camera, bool active = false) : Camera(camera), IsActive(active) {}
		CameraComponent(const CameraComponent&) = default;
	};

	struct DirectionalLightComponent
	{
		Vector3f Color = Vector3f(1.0f);
		float Intensity = 5.0f;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const Vector3f& color, float intensity)
			: Color(color), Intensity(intensity) { }
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
	};

	struct SpotLightComponent
	{
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
		std::vector<Matrix4f> BoneMatrices = std::vector<Matrix4f>(Constants::Renderer::MaxBones, Matrix4f(1.0f));

		AnimatorComponent() = default;
		AnimatorComponent(UUID skeletonUUID, UUID animationUUID)
			: SkeletonHandle(skeletonUUID), CurrentAnimationHandle(animationUUID) {}
		AnimatorComponent(const AnimatorComponent&) = default;
	};

}