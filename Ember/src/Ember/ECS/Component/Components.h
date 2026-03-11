#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"

namespace Ember {
	class SceneEntity;
	class ScriptableEntity;
}

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
		Vector3f Size;

		TransformComponent(const Vector3f& position, const Vector3f& size = Vector3f(1.0f)) : Position(position), Size(size) {}
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

	struct CameraComponent
	{
		Camera Camera;
		bool IsActive;

		CameraComponent(const Ember::Camera& camera, bool active = false) : Camera(camera), IsActive(active) {}
	};

	struct ScriptComponent
	{
		bool Initalized = false;

		// Inline Lambda Function //
		std::function<void(SceneEntity)> OnCreate = nullptr;
		std::function<void(SceneEntity, TimeStep)> OnUpdate = nullptr;
		std::function<void(SceneEntity)> OnDestroy = nullptr;

		// Class Binding 
		ScriptableEntity* Instance = nullptr;
		ScriptableEntity* (*CreateScript)() = nullptr;
		void (*DestroyScript)(ScriptComponent*) = nullptr;

		template<typename T>
		void Bind()
		{
			CreateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](ScriptComponent* sc) { delete sc->Instance; sc->Instance = nullptr; };
		}

		~ScriptComponent()
		{
			if (Instance && DestroyScript)
			{
				DestroyScript(this);
			}
		}
	};

}