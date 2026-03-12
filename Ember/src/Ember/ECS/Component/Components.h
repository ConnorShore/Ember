#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"
#include "Ember/Render/Mesh.h"
#include "Ember/Render/Material.h"

namespace Ember {
	class Entity;
	class Behavior;
}

#include <memory>
#include <string>
#include <functional>

namespace Ember {

	struct TagComponent
	{
		std::string Tag;

		TagComponent(const std::string& tag) : Tag(tag) {}
	};

	struct TransformComponent
	{
		Vector3f Position;
		Vector3f Rotation;
		Vector3f Size;

		TransformComponent(const Vector3f& position = Vector3f(0.0f),
			const Vector3f& rotation = Vector3f(0.0f),
			const Vector3f& size = Vector3f(1.0f))
			: Position(position), Rotation(rotation), Size(size) {
		}

		Matrix4f GetTransformationMatrix() const
		{
			return Math::Translate(Position) * Math::GetRotationMatrix(Rotation) * Math::Scale(Size);
		}

		Vector3f GetForward() const
		{
			Ember::Quaternion q(Rotation);
			return Math::Normalize(q * Ember::Vector3f(0.0f, 0.0f, -1.0f));
		}

		Vector3f GetRight() const
		{
			Ember::Quaternion q(Rotation);
			return Math::Normalize(q * Ember::Vector3f(1.0f, 0.0f, 0.0f));
		}

		Vector3f GetUp() const
		{
			Ember::Quaternion q(Rotation);
			return Math::Normalize(q * Ember::Vector3f(0.0f, 1.0f, 0.0f));
		}
	};

	struct RigidBodyComponent
	{
		Vector3f Velocity;

		RigidBodyComponent(const Vector3f& velocity) : Velocity(velocity) {}
	};

	struct SpriteComponent
	{
		Vector4f Color;
		SharedPtr<Texture> Texture;

		SpriteComponent(const Vector4f color) : Color(color) {}
		SpriteComponent(const SharedPtr<Ember::Texture>& texture) : Color(Vector4f(1.0f)), Texture(texture) {}
	};

	struct MeshComponent
	{
		SharedPtr<Mesh> Mesh;

		MeshComponent(const SharedPtr<Ember::Mesh>& mesh) : Mesh(mesh) {}
	};

	struct MaterialComponent
	{
		SharedPtr<MaterialBase> Material;

		MaterialComponent(const SharedPtr<Ember::MaterialBase>& material) : Material(material) {}

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

	struct MaterialComponentOld
	{
		// Will be a single material ptr in the future
		SharedPtr<Shader> Shader;	
		SharedPtr<Texture> Texture;
		Vector4f TintColor;

		MaterialComponentOld(const SharedPtr<Ember::Shader>& shader, const SharedPtr<Ember::Texture> texture, Vector4f tintColor = { 1.0f, 1.0f, 1.0f, 1.0f })
			: Shader(shader), Texture(texture), TintColor(tintColor) { }
	};

	struct CameraComponent
	{
		Camera Camera;
		bool IsActive;

		CameraComponent() = default;
		CameraComponent(const Ember::Camera& camera, bool active = false) : Camera(camera), IsActive(active) {}
	};

	struct ScriptComponent
	{
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
		void Bind()
		{
			OnInitScript = []() { return static_cast<Behavior*>(new T()); };
			OnDestroyScript = [](ScriptComponent* sc) { delete sc->Instance; sc->Instance = nullptr; };
		}

		~ScriptComponent()
		{
			if (Instance && OnDestroyScript)
			{
				OnDestroyScript(this);
			}
		}
	};

}