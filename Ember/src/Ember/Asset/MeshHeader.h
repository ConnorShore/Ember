#pragma once

#include "UUID.h"
#include "Ember/Math/Math.h"

#include <glm/glm.hpp>
#include <cstdint>

namespace Ember {

	// Magic number to verify the file type: "EBMH" (Ember Mesh Header)
	constexpr uint32_t MESH_FILE_MAGIC = 0x484D4245;

	struct MeshVertex
	{
		Vector3f Position;
		Vector3f Normal;
		Vector2f TexCoords;
		Vector3f Tangent;
		Vector3f Bitangent;
	};

	// We use #pragma pack(push, 1) to ensure the compiler doesn't add padding bytes 
	// between the variables, which would corrupt our binary read/writes
#pragma pack(push, 1)
	struct MeshHeader
	{
		uint32_t MagicNumber = MESH_FILE_MAGIC;
		uint32_t Version = 1;

		uint32_t VertexCount = 0;
		uint32_t IndexCount = 0;

		// Bounding Box (Used later for Frustum Culling)
		struct {
			float Min[3];
			float Max[3];
		} Bounds = {};
	};
#pragma pack(pop)

}