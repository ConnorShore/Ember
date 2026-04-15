#pragma once

#include "Ember/Core/Core.h"
#include "StaticMesh.h"

#include <vector>
namespace Ember {

	class PrimitiveGenerator
	{
	public:
		static SharedPtr<StaticMesh> CreateSphere(const std::string& name = "Primitive_Sphere", float size = 1.0f, uint32_t xSegments = 64, uint32_t ySegments = 64);
		static SharedPtr<StaticMesh> CreateCube(const std::string& name = "Primitive_Cube", float size = 1.0f);

		static SharedPtr<StaticMesh> CreateQuad(const std::string& name = "Primitive_Quad", float width = 1.0f, float height = 1.0f);
		static SharedPtr<StaticMesh> CreateQuad(float width, float height);

		static SharedPtr<StaticMesh> CreateCapsule(const std::string& name = "Primitive_Capsule", float radius = 0.5f, float height = 2.0f);
	};

}