#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/Camera.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Texture.h"

#include <string>

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

}