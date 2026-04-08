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

#include <sol/sol.hpp>

#include <memory>
#include <string>
#include <functional>

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
		Vector3f Velocity = Vector3f(0.0f);

		RigidBodyComponent() = default;
		RigidBodyComponent(const Vector3f& velocity) : Velocity(velocity) {}
		RigidBodyComponent(const RigidBodyComponent&) = default;
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
		EntityID RootAnimator = Constants::Entities::InvalidEntityID;

		SkinnedMeshComponent() = default;
		SkinnedMeshComponent(UUID meshId, EntityID rootAnimator = Constants::Entities::InvalidEntityID)
			: MeshHandle(meshId), RootAnimator(rootAnimator) {
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

			assetManager.Register(newInstance);

			MaterialHandle = newInstance->GetUUID();

			return newInstance;
		}
	};

	struct CameraComponent
	{
		Camera Camera;
		bool IsActive;

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
		float Size = 1.0f;

		BillboardComponent() = default;
		BillboardComponent(const BillboardComponent&) = default;
	};

	struct AnimatorComponent
	{
		UUID SkeletonHandle = Constants::InvalidUUID;
		UUID CurrentAnimationHandle = Constants::InvalidUUID;

		std::vector<Matrix4f> BoneMatrices;

		TimeStep CurrentTime = 0.0f;
		bool IsPlaying = true;
		bool Loop = true;

		AnimatorComponent()
		{
			BoneMatrices.resize(Constants::Renderer::MaxBones, Matrix4f(1.0f));
		}
	};

}