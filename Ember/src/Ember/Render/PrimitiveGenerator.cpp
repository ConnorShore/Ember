#include "ebpch.h"
#include "PrimitiveGenerator.h"
#include <cmath>

namespace Ember {

	SharedPtr<Mesh> PrimitiveGenerator::CreateSphere(const std::string& name, float size, unsigned int xSegments, unsigned int ySegments)
	{
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		const float PI = 3.14159265359f;
		float radius = size * 0.5f;

		// 1. Generate Vertices (14 Floats per vertex)
		for (unsigned int y = 0; y <= ySegments; ++y)
		{
			for (unsigned int x = 0; x <= xSegments; ++x)
			{
				float xSegment = (float)x / (float)xSegments;
				float ySegment = (float)y / (float)ySegments;

				// Calculate base unit sphere positions
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				// Position (Scaled by radius)
				vertices.push_back(xPos * radius);
				vertices.push_back(yPos * radius);
				vertices.push_back(zPos * radius);

				// Normal (For a sphere at origin, normalized position IS the normal)
				vertices.push_back(xPos);
				vertices.push_back(yPos);
				vertices.push_back(zPos);

				// Texture Coordinates (UV)
				vertices.push_back(xSegment);
				vertices.push_back(ySegment);

				// Tangents (Points along the U axis/longitude)
				vertices.push_back(-std::sin(xSegment * 2.0f * PI));
				vertices.push_back(0.0f);
				vertices.push_back(std::cos(xSegment * 2.0f * PI));

				// Bitangents (Points along the V axis/latitude)
				vertices.push_back(-std::cos(xSegment * 2.0f * PI) * std::cos(ySegment * PI));
				vertices.push_back(std::sin(ySegment * PI));
				vertices.push_back(-std::sin(xSegment * 2.0f * PI) * std::cos(ySegment * PI));
			}
		}

		// 2. Generate Indices
		for (unsigned int y = 0; y < ySegments; ++y)
		{
			for (unsigned int x = 0; x < xSegments; ++x)
			{
				unsigned int top_left = (y * (xSegments + 1)) + x;
				unsigned int top_right = top_left + 1;
				unsigned int bottom_left = ((y + 1) * (xSegments + 1)) + x;
				unsigned int bottom_right = bottom_left + 1;

				// Triangle 1
				indices.push_back(top_left);
				indices.push_back(bottom_left);
				indices.push_back(top_right);

				// Triangle 2
				indices.push_back(top_right);
				indices.push_back(bottom_left);
				indices.push_back(bottom_right);
			}
		}

		return SharedPtr<Mesh>::Create(name, vertices, indices);
	}

	SharedPtr<Mesh> PrimitiveGenerator::CreateCube(const std::string& name, float size)
	{
		float s = size * 0.5f;

		std::vector<float> vertices = {
			// Position    // Normal              // UV         // Tangent            // Bitangent

			// Front Face (Normal +Z, U moves +X, V moves +Y)
			-s, -s,  s,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			 s, -s,  s,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			 s,  s,  s,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			-s,  s,  s,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,   1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
																					 
			// Right Face (Normal +X, U moves -Z, V moves +Y)						 
			 s, -s,  s,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,
			 s, -s, -s,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,
			 s,  s, -s,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,   0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,
			 s,  s,  s,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,   0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,
																					 
			// Back Face (Normal -Z, U moves -X, V moves +Y)						 
			 s, -s, -s,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			-s, -s, -s,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			-s,  s, -s,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
			 s,  s, -s,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,  -1.0f,  0.0f,  0.0f,   0.0f,  1.0f,  0.0f,
																					 
			// Left Face (Normal -X, U moves +Z, V moves +Y)						 
			-s, -s, -s,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,   0.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,
			-s, -s,  s,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,   0.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,
			-s,  s,  s,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,   0.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,
			-s,  s, -s,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,   0.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,

			// Top Face (Normal +Y, U moves +X, V moves -Z)
			-s,  s,  s,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,
			 s,  s,  s,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,
			 s,  s, -s,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,
			-s,  s, -s,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f,

			// Bottom Face (Normal -Y, U moves +X, V moves +Z)
			-s, -s, -s,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			 s, -s, -s,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			 s, -s,  s,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			-s, -s,  s,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,   1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f
		};

		std::vector<unsigned int> indices = {
			 0,  1,  2,  2,  3,  0, // Front
			 4,  5,  6,  6,  7,  4, // Right
			 8,  9, 10, 10, 11,  8, // Back
			12, 13, 14, 14, 15, 12, // Left
			16, 17, 18, 18, 19, 16, // Top
			20, 21, 22, 22, 23, 20  // Bottom
		};

		return SharedPtr<Mesh>::Create(name, vertices, indices);
	}

	SharedPtr<Mesh> PrimitiveGenerator::CreateQuad(const std::string& name, float width, float height)
	{
		float hw = width * 0.5f;
		float hh = height * 0.5f;

		std::vector<float> vertices = {
			// Position         // Normal           // UV         // Tangent           // Bitangent
			-hw, -hh, 0.0f,     0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
			 hw, -hh, 0.0f,     0.0f, 0.0f, 1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
			 hw,  hh, 0.0f,     0.0f, 0.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
			-hw,  hh, 0.0f,     0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f
		};

		std::vector<unsigned int> indices = {
			0, 1, 2, 2, 3, 0
		};

		return SharedPtr<Mesh>::Create(name, vertices, indices);
	}


	SharedPtr<Mesh> PrimitiveGenerator::CreateQuad(float width, float height)
	{
		return CreateQuad("Primitive_Quad", width, height);
	}

}