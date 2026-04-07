#pragma once

#include "UUID.h"
#include "MeshHeader.h"

#include "Ember/Render/StaticMesh.h"
#include "Ember/Core/Core.h"

#include <fstream>
#include <filesystem>

namespace Ember {

	// Reads a binary .ebmesh file: validates the magic/version header, then
	// bulk-reads vertex and index data directly into memory.
	class MeshSerializer
	{
	public:
		static SharedPtr<StaticMesh> Deserialize(UUID uuid, const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open())
			{
				EB_CORE_ERROR("Failed to open cooked mesh file: {0}", filepath.string());
				return nullptr;
			}

			// Read and Validate Header
			MeshHeader header;
			file.read((char*)&header, sizeof(MeshHeader));

			if (header.MagicNumber != MESH_FILE_MAGIC)
			{
				EB_CORE_ERROR("Invalid mesh file format: {0}", filepath.string());
				return nullptr;
			}

			if (header.Version != 1)
			{
				EB_CORE_ERROR("Unsupported mesh file version in: {0}", filepath.string());
				return nullptr;
			}

			// Allocate memory for the data
			std::vector<StaticMeshVertex> vertices(header.VertexCount);
			std::vector<uint32_t> indices(header.IndexCount);

			// Read data straight from disk into our vectors
			size_t vertexDataSize = header.VertexCount * sizeof(StaticMeshVertex);
			file.read((char*)vertices.data(), vertexDataSize);

			size_t indexDataSize = header.IndexCount * sizeof(uint32_t);
			file.read((char*)indices.data(), indexDataSize);

			file.close();

			std::string name = filepath.stem().string();

			auto meshAsset = SharedPtr<StaticMesh>::Create(uuid, name, vertices, indices);
			meshAsset->SetFilePath(filepath.string());
			return meshAsset;
		}
	};
}