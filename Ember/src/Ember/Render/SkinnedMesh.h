#pragma once

#include "Mesh.h"
#include "Ember/Asset/MeshHeader.h"

namespace Ember {

	class SkinnedMesh : public Mesh
	{
	public:
		SkinnedMesh(const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		SkinnedMesh(UUID uuid, const std::string& name, const std::vector<SkinnedMeshVertex>& vertices, const std::vector<uint32_t>& indices);
		SkinnedMesh(UUID uuid, const std::string& name, const std::vector<float>& vertices, const std::vector<uint32_t>& indices);
		virtual ~SkinnedMesh();

		inline UUID GetSkeletonHandle() const { return m_SkeletonHandle; }
		inline void SetSkeletonHandle(const UUID& skeletonHandle) { m_SkeletonHandle = skeletonHandle; }

	private:
		UUID m_SkeletonHandle;
	};
}