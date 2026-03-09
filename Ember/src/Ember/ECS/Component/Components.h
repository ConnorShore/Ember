#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"
#include "Ember/Render/VertexArray.h"
#include "Ember/Render/Shader.h"
#include "Ember/Render/Camera.h"

#include <string>

#define EB_DEFAULT_COMPONENT_CONSTRUCT(type) type() = default; \
                                                type(const type& other) = default; \
                                                ~type() = default

namespace Ember {

	struct TagComponent
	{
		std::string Tag;

		EB_DEFAULT_COMPONENT_CONSTRUCT(TagComponent);
		TagComponent(const std::string& tag) : Tag(tag) {}
	};

	struct TransformComponent
	{
		Vector3f Position;
		Vector3f Size;

		EB_DEFAULT_COMPONENT_CONSTRUCT(TransformComponent);
		TransformComponent(const Vector3f& position, const Vector3f& size = Vector3f(1.0f)) : Position(position), Size(size) {}
	};

	struct RigidBodyComponent
	{
		Vector3f Velocity;

		EB_DEFAULT_COMPONENT_CONSTRUCT(RigidBodyComponent);
		RigidBodyComponent(const Vector3f& velocity) : Velocity(velocity) {}
	};

	struct SpriteComponent
	{
		Vector4f Color;

		EB_DEFAULT_COMPONENT_CONSTRUCT(SpriteComponent);
		SpriteComponent(const Vector4f color) : Color(color) {}
	};

	struct CameraComponent
	{
		Matrix4f ProjectionMatrix;
		bool IsActive;

		EB_DEFAULT_COMPONENT_CONSTRUCT(CameraComponent);
		CameraComponent(const Matrix4f& projection, bool active = false) : ProjectionMatrix(projection), IsActive(active) {}
	};

}