#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"
#include "Ember/ECS/Types.h"
#include "Ember/Core/Constants.h"

namespace Ember {
	class Entity;
	class Behavior;
}

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
		UUID ParentHandle = Constants::Entities::InvalidEntityUUID;
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
		Vector4f Color;
		SharedPtr<Texture> Texture;

		SpriteComponent(const Vector4f color) : Color(color) {}
		SpriteComponent(const SharedPtr<Ember::Texture>& texture) : Color(Vector4f(1.0f)), Texture(texture) {}
		SpriteComponent(const Vector4f color, const SharedPtr<Ember::Texture>& texture) : Color(color), Texture(texture) {}
	};

	struct MeshComponent
	{
		SharedPtr<Mesh> Mesh;

		MeshComponent() = default;
		MeshComponent(const SharedPtr<Ember::Mesh>& mesh) : Mesh(mesh) {}
		MeshComponent(const MeshComponent&) = default;
	};

	struct MaterialComponent
	{
		SharedPtr<MaterialBase> Material;

		MaterialComponent() = default;
		MaterialComponent(const SharedPtr<Ember::MaterialBase>& material) : Material(material) {}
		MaterialComponent(const MaterialComponent&) = default;

		SharedPtr<MaterialInstance> GetInstanced()
		{
			// If already an instance, return it
			if (auto instance = DynamicPointerCast<MaterialInstance>(Material))
				return instance;

			// Convert material to an instance
			if (auto base = DynamicPointerCast<Ember::Material>(Material))
			{
				auto newInstance = SharedPtr<MaterialInstance>::Create(base->GetName(), base);
				Material = newInstance;
				return newInstance;
			}

			EB_CORE_ASSERT(false, "Unknown Material type!");
			return nullptr;
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

		float CutOffAngle = Math::Radians(12.5f); // Stored in Radians for the C++ Camera
		float OuterCutOffAngle = Math::Radians(17.5f); // Stored in Radians for the C++ Camera

		float CutOff = cos(Math::Radians(12.5f));      // Stored as Cosine for the GLSL Shader
		float OuterCutOff = cos(Math::Radians(17.5f));; // Stored as Cosine for the GLSL Shader

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
		float Intensity = 50.0f;
		float Radius = 0.0f;

		PointLightComponent() = default;
		PointLightComponent(const Vector3f& color, float intensity, float radius)
			: Color(color), Intensity(intensity), Radius(radius) { }
		PointLightComponent(const PointLightComponent&) = default;
	};

	struct ScriptComponent
	{
		std::string ClassName = "";
		bool Initalized = false;

		// Inline Lambda Function //
		std::function<void(Entity)> OnCreate = nullptr;
		std::function<void(Entity, TimeStep)> OnUpdate = nullptr;
		std::function<void(Entity)> OnDestroy = nullptr;

		// Class Binding 
		Behavior* Instance = nullptr;
		Behavior* (*OnInitScript)() = nullptr;
		void (*OnDestroyScript)(ScriptComponent*) = nullptr;

		template<typename T>
		void Bind(const std::string& className)
		{
			ClassName = className;
			OnInitScript = []() { return static_cast<Behavior*>(new T()); };
			OnDestroyScript = [](ScriptComponent* sc) { delete sc->Instance; sc->Instance = nullptr; };
		}

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
		~ScriptComponent()
		{
			if (Instance && OnDestroyScript)
			{
				OnDestroyScript(this);
			}
		}
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

}