#pragma once

#include "Mesh.h"
#include "Ember/Asset/MeshHeader.h"

namespace Ember {

	class StaticMesh : public Mesh
	{
	public:
		StaticMesh(const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		StaticMesh(UUID uuid, const std::string& name, const std::vector<StaticMeshVertex>& vertices, const std::vector<uint32_t>& indices);
		StaticMesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		virtual ~StaticMesh();
	};
}