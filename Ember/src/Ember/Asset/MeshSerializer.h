#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Asset/MeshHeader.h"
#include "Ember/Render/StaticMesh.h"
#include "Ember/Render/SkinnedMesh.h"

#include <fstream>
#include <filesystem>

namespace Ember {

	class MeshSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const std::vector<SkinnedMeshVertex>& vertices, const std::vector<uint32_t>& indices, bool isSkinned)
		{
			std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
			if (!file.is_open()) return false;

			MeshHeader header;
			header.MagicNumber = MESH_FILE_MAGIC;
			header.Version = 1;
			header.VertexCount = (uint32_t)vertices.size();
			header.IndexCount = (uint32_t)indices.size();
			header.IsSkinned = isSkinned;

			// Calculate Bounds directly from the temporary vectors
			Vector3f min(FLT_MAX), max(-FLT_MAX);
			for (const auto& v : vertices) {
				min = Math::Min(min, v.Position);
				max = Math::Max(max, v.Position);
			}
			header.Bounds.Min[0] = min.x; header.Bounds.Min[1] = min.y; header.Bounds.Min[2] = min.z;
			header.Bounds.Max[0] = max.x; header.Bounds.Max[1] = max.y; header.Bounds.Max[2] = max.z;

			file.write((char*)&header, sizeof(MeshHeader));

			if (isSkinned) {
				file.write((char*)vertices.data(), header.VertexCount * sizeof(SkinnedMeshVertex));
			}
			else {
				// Inline downcast to StaticMeshVertex to save disk space
				for (const auto& v : vertices) {
					StaticMeshVertex sv = { v.Position, v.Normal, v.TexCoords, v.Tangent, v.Bitangent };
					file.write((char*)&sv, sizeof(StaticMeshVertex));
				}
			}

			file.write((char*)indices.data(), header.IndexCount * sizeof(uint32_t));
			file.close();
		}

		static SharedPtr<Mesh> Deserialize(UUID uuid, const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open()) return nullptr;

			MeshHeader header;
			file.read((char*)&header, sizeof(MeshHeader));

			if (header.MagicNumber != MESH_FILE_MAGIC) return nullptr;

			std::string name = filepath.stem().string();
			if (header.IsSkinned) {
				std::vector<SkinnedMeshVertex> verts(header.VertexCount);
				file.read((char*)verts.data(), header.VertexCount * sizeof(SkinnedMeshVertex));

				std::vector<uint32_t> indices(header.IndexCount);
				file.read((char*)indices.data(), header.IndexCount * sizeof(uint32_t));

				return SharedPtr<SkinnedMesh>::Create(uuid, name, verts, indices);
			}
			else {
				std::vector<StaticMeshVertex> verts(header.VertexCount);
				file.read((char*)verts.data(), header.VertexCount * sizeof(StaticMeshVertex));

				std::vector<uint32_t> indices(header.IndexCount);
				file.read((char*)indices.data(), header.IndexCount * sizeof(uint32_t));

				return SharedPtr<StaticMesh>::Create(uuid, name, verts, indices);
			}
		}
	};
}