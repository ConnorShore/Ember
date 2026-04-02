#pragma once

#include "Ember/Core/Core.h"
#include "Mesh.h"

#include <vector>
namespace Ember {

	class PrimitiveGenerator
	{
	public:
		static SharedPtr<Mesh> CreateSphere(const std::string& name = "Primitive_Sphere", float size = 1.0f, unsigned int xSegments = 64, unsigned int ySegments = 64);
		static SharedPtr<Mesh> CreateCube(const std::string& name = "Primitive_Cube", float size = 1.0f);
		static SharedPtr<Mesh> CreateQuad(const std::string& name = "Primitive_Quad", float width = 1.0f, float height = 1.0f);
		static SharedPtr<Mesh> CreateQuad(float width, float height);
	};

}