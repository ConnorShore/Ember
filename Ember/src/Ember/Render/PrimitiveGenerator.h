#pragma once

#include "Ember/Core/Core.h"
#include "Mesh.h"

#include <vector>
namespace Ember {

	class PrimitiveGenerator
	{
	public:
		static SharedPtr<Mesh> CreateSphere(float radius = 1.0f, unsigned int xSegments = 64, unsigned int ySegments = 64);
		static SharedPtr<Mesh> CreateCube(float size = 1.0f);
		static SharedPtr<Mesh> CreateQuad(float width = 1.0f, float height = 1.0f);
	};

}