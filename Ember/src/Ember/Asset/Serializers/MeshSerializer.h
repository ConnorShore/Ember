#pragma once

#include "Ember/Core/Core.h"
#include "AssetSerializationMode.h"
#include "Ember/Asset/MeshHeader.h"
#include "Ember/Render/StaticMesh.h"
#include "Ember/Render/SkinnedMesh.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>
#include <filesystem>

namespace Ember {
	namespace {
		inline bool MeshFileLooksBinary(const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open())
				return false;

			uint32_t magic = 0;
			file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
			return file.good() && magic == MESH_FILE_MAGIC;
		}

		// Cooked binary sibling for a mesh source path (foo.ebmesh -> foo.bin).
		inline std::filesystem::path GetCookedMeshPath(const std::filesystem::path& filepath)
		{
			auto cooked = filepath;
			cooked.replace_extension(".bin");
			return cooked;
		}

		// A cooked mesh is safe to use if it exists and is at least as new as its source, so that
		// re-importing/editing the source (which rewrites the .ebmesh) invalidates a stale cook.
		inline bool IsCookedMeshFresh(const std::filesystem::path& source, const std::filesystem::path& cooked)
		{
			std::error_code ec;
			if (!std::filesystem::exists(cooked, ec) || ec)
				return false;

			auto cookedTime = std::filesystem::last_write_time(cooked, ec);
			if (ec)
				return false;

			auto sourceTime = std::filesystem::last_write_time(source, ec);
			if (ec)
				return true; // Source unreadable but the cook exists — prefer the cook over failing.

			return cookedTime >= sourceTime;
		}
	}

	class MeshSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Mesh>& mesh)
		{
			if (!mesh)
				return false;

			auto outputPath = filepath;
			outputPath.replace_extension(".ebmesh");

			const auto& vertexBuffer = mesh->GetVertexArray()->GetVertexBuffer();
			const auto& indexBuffer = mesh->GetVertexArray()->GetIndexBuffer();
			const bool isSkinned = DynamicPointerCast<SkinnedMesh>(mesh) != nullptr;

			const uint32_t vertexCount = mesh->GetVertexCount();
			const uint32_t indexCount = static_cast<uint32_t>(indexBuffer->GetCount());
			const uint8_t* rawVertexData = reinterpret_cast<const uint8_t*>(vertexBuffer->GetData());

			ryml::Tree tree;
			auto root = tree.rootref();
			root |= ryml::MAP;
			root["Version"] << 1;
			root["Mesh"] << mesh->GetName();
			root["UUID"] << static_cast<uint64_t>(mesh->GetUUID());
			root["IsSkinned"] << isSkinned;

			auto boundsNode = root["Bounds"];
			boundsNode |= ryml::MAP;
			auto minNode = boundsNode["Min"];
			minNode |= ryml::SEQ | ryml::FLOW_SL;
			minNode.append_child() << mesh->GetMinBounds().x;
			minNode.append_child() << mesh->GetMinBounds().y;
			minNode.append_child() << mesh->GetMinBounds().z;
			auto maxNode = boundsNode["Max"];
			maxNode |= ryml::SEQ | ryml::FLOW_SL;
			maxNode.append_child() << mesh->GetMaxBounds().x;
			maxNode.append_child() << mesh->GetMaxBounds().y;
			maxNode.append_child() << mesh->GetMaxBounds().z;

			auto verticesNode = root["Vertices"];
			verticesNode |= ryml::SEQ;
			if (isSkinned)
			{
				const auto* vertices = reinterpret_cast<const SkinnedMeshVertex*>(rawVertexData);
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					auto vNode = verticesNode.append_child();
					vNode |= ryml::MAP;
					auto writeVec2 = [&](ryml::NodeRef node, const Vector2f& v) {
						node |= ryml::SEQ | ryml::FLOW_SL;
						node.append_child() << v.x;
						node.append_child() << v.y;
					};
					auto writeVec3 = [&](ryml::NodeRef node, const Vector3f& v) {
						node |= ryml::SEQ | ryml::FLOW_SL;
						node.append_child() << v.x;
						node.append_child() << v.y;
						node.append_child() << v.z;
					};
					auto writeVec4 = [&](ryml::NodeRef node, const Vector4f& v) {
						node |= ryml::SEQ | ryml::FLOW_SL;
						node.append_child() << v.x;
						node.append_child() << v.y;
						node.append_child() << v.z;
						node.append_child() << v.w;
					};

					writeVec3(vNode["Position"], vertices[i].Position);
					writeVec3(vNode["Normal"], vertices[i].Normal);
					writeVec2(vNode["TexCoords"], vertices[i].TexCoords);
					writeVec3(vNode["Tangent"], vertices[i].Tangent);
					writeVec3(vNode["Bitangent"], vertices[i].Bitangent);

					auto boneNode = vNode["BoneIDs"];
					boneNode |= ryml::SEQ | ryml::FLOW_SL;
					boneNode.append_child() << vertices[i].BoneIDs.x;
					boneNode.append_child() << vertices[i].BoneIDs.y;
					boneNode.append_child() << vertices[i].BoneIDs.z;
					boneNode.append_child() << vertices[i].BoneIDs.w;

					writeVec4(vNode["BoneWeights"], vertices[i].BoneWeights);
				}
			}
			else
			{
				const auto* vertices = reinterpret_cast<const StaticMeshVertex*>(rawVertexData);
				for (uint32_t i = 0; i < vertexCount; i++)
				{
					auto vNode = verticesNode.append_child();
					vNode |= ryml::MAP;
					auto writeVec2 = [&](ryml::NodeRef node, const Vector2f& v) {
						node |= ryml::SEQ | ryml::FLOW_SL;
						node.append_child() << v.x;
						node.append_child() << v.y;
					};
					auto writeVec3 = [&](ryml::NodeRef node, const Vector3f& v) {
						node |= ryml::SEQ | ryml::FLOW_SL;
						node.append_child() << v.x;
						node.append_child() << v.y;
						node.append_child() << v.z;
					};
					writeVec3(vNode["Position"], vertices[i].Position);
					writeVec3(vNode["Normal"], vertices[i].Normal);
					writeVec2(vNode["TexCoords"], vertices[i].TexCoords);
					writeVec3(vNode["Tangent"], vertices[i].Tangent);
					writeVec3(vNode["Bitangent"], vertices[i].Bitangent);
				}
			}

			auto indicesNode = root["Indices"];
			indicesNode |= ryml::SEQ | ryml::FLOW_SL;
			const auto* idx = reinterpret_cast<const uint32_t*>(indexBuffer->GetData());
			for (uint32_t i = 0; i < indexCount; i++)
				indicesNode.append_child() << idx[i];

			std::ofstream fout(outputPath);
			if (!fout.is_open())
				return false;
			fout << tree;
			return true;
		}

		static SharedPtr<Mesh> DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
		{
			std::ifstream stream(filepath);
			if (!stream.is_open())
				return nullptr;

			std::stringstream ss;
			ss << stream.rdbuf();
			auto yamlData = ss.str();
			ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
			auto root = tree.rootref();

			bool isSkinned = false;
			if (root.has_child("IsSkinned"))
				root["IsSkinned"] >> isSkinned;

			std::vector<uint32_t> indices;
			if (root.has_child("Indices"))
			{
				auto indicesNode = root["Indices"];
				indices.reserve(indicesNode.num_children());
				for (auto idxNode : indicesNode.children())
				{
					uint32_t idx = 0;
					idxNode >> idx;
					indices.push_back(idx);
				}
			}

			if (isSkinned)
			{
				std::vector<SkinnedMeshVertex> vertices;
				if (root.has_child("Vertices"))
				{
					vertices.reserve(root["Vertices"].num_children());
					for (auto vNode : root["Vertices"].children())
					{
						SkinnedMeshVertex vertex{};
						auto readVec2 = [&](ryml::NodeRef node, Vector2f& v) {
							if (node.valid() && node.is_seq() && node.num_children() == 2) {
								node[0] >> v.x; node[1] >> v.y;
							}
						};
						auto readVec3 = [&](ryml::NodeRef node, Vector3f& v) {
							if (node.valid() && node.is_seq() && node.num_children() == 3) {
								node[0] >> v.x; node[1] >> v.y; node[2] >> v.z;
							}
						};
						auto readVec4 = [&](ryml::NodeRef node, Vector4f& v) {
							if (node.valid() && node.is_seq() && node.num_children() == 4) {
								node[0] >> v.x; node[1] >> v.y; node[2] >> v.z; node[3] >> v.w;
							}
						};
						readVec3(vNode["Position"], vertex.Position);
						readVec3(vNode["Normal"], vertex.Normal);
						readVec2(vNode["TexCoords"], vertex.TexCoords);
						readVec3(vNode["Tangent"], vertex.Tangent);
						readVec3(vNode["Bitangent"], vertex.Bitangent);
						if (vNode.has_child("BoneIDs"))
						{
							auto b = vNode["BoneIDs"];
							if (b.is_seq() && b.num_children() == 4)
							{
								b[0] >> vertex.BoneIDs.x;
								b[1] >> vertex.BoneIDs.y;
								b[2] >> vertex.BoneIDs.z;
								b[3] >> vertex.BoneIDs.w;
							}
						}
						readVec4(vNode["BoneWeights"], vertex.BoneWeights);
						vertices.push_back(vertex);
					}
				}

				auto mesh = SharedPtr<SkinnedMesh>::Create(uuid, filepath.stem().string(), vertices, indices);
				mesh->SetFilePath(filepath.string());
				return mesh;
			}

			std::vector<StaticMeshVertex> vertices;
			if (root.has_child("Vertices"))
			{
				vertices.reserve(root["Vertices"].num_children());
				for (auto vNode : root["Vertices"].children())
				{
					StaticMeshVertex vertex{};
					auto readVec2 = [&](ryml::NodeRef node, Vector2f& v) {
						if (node.valid() && node.is_seq() && node.num_children() == 2) {
							node[0] >> v.x; node[1] >> v.y;
						}
					};
					auto readVec3 = [&](ryml::NodeRef node, Vector3f& v) {
						if (node.valid() && node.is_seq() && node.num_children() == 3) {
							node[0] >> v.x; node[1] >> v.y; node[2] >> v.z;
						}
					};
					readVec3(vNode["Position"], vertex.Position);
					readVec3(vNode["Normal"], vertex.Normal);
					readVec2(vNode["TexCoords"], vertex.TexCoords);
					readVec3(vNode["Tangent"], vertex.Tangent);
					readVec3(vNode["Bitangent"], vertex.Bitangent);
					vertices.push_back(vertex);
				}
			}

			auto mesh = SharedPtr<StaticMesh>::Create(uuid, filepath.stem().string(), vertices, indices);
			mesh->SetFilePath(filepath.string());
			return mesh;
		}

		static bool SerializeCooked(const std::filesystem::path& filepath, const std::vector<SkinnedMeshVertex>& vertices, const std::vector<uint32_t>& indices, bool isSkinned)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			std::ofstream file(cookedPath, std::ios::binary | std::ios::trunc);
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
			return true;
		}

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Mesh>& mesh)
		{
			if (!mesh)
				return false;
			const bool isSkinned = DynamicPointerCast<SkinnedMesh>(mesh) != nullptr;
			const auto* vbRaw = reinterpret_cast<const uint8_t*>(mesh->GetVertexArray()->GetVertexBuffer()->GetData());
			std::vector<uint32_t> indices = mesh->GetTriangles();

			if (isSkinned)
			{
				std::vector<SkinnedMeshVertex> vertices(mesh->GetVertexCount());
				memcpy(vertices.data(), vbRaw, sizeof(SkinnedMeshVertex) * vertices.size());
				return SerializeCooked(filepath, vertices, indices, true);
			}

			std::vector<SkinnedMeshVertex> promoted(mesh->GetVertexCount());
			auto* staticVertices = reinterpret_cast<const StaticMeshVertex*>(vbRaw);
			for (size_t i = 0; i < promoted.size(); i++)
			{
				promoted[i].Position = staticVertices[i].Position;
				promoted[i].Normal = staticVertices[i].Normal;
				promoted[i].TexCoords = staticVertices[i].TexCoords;
				promoted[i].Tangent = staticVertices[i].Tangent;
				promoted[i].Bitangent = staticVertices[i].Bitangent;
			}
			return SerializeCooked(filepath, promoted, indices, false);
		}

		static SharedPtr<Mesh> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open()) return nullptr;

			MeshHeader header;
			file.read((char*)&header, sizeof(MeshHeader));

			if (header.MagicNumber != MESH_FILE_MAGIC) return nullptr;

			std::string name = filepath.stem().string();
			SharedPtr<Mesh> mesh = nullptr;
			if (header.IsSkinned) {
				std::vector<SkinnedMeshVertex> verts(header.VertexCount);
				file.read((char*)verts.data(), header.VertexCount * sizeof(SkinnedMeshVertex));

				std::vector<uint32_t> indices(header.IndexCount);
				file.read((char*)indices.data(), header.IndexCount * sizeof(uint32_t));

				mesh = SharedPtr<SkinnedMesh>::Create(uuid, name, verts, indices);
			}
			else {
				std::vector<StaticMeshVertex> verts(header.VertexCount);
				file.read((char*)verts.data(), header.VertexCount * sizeof(StaticMeshVertex));

				std::vector<uint32_t> indices(header.IndexCount);
				file.read((char*)indices.data(), header.IndexCount * sizeof(uint32_t));

				mesh = SharedPtr<StaticMesh>::Create(uuid, name, verts, indices);
			}

			mesh->SetFilePath(filepath.string());
			//mesh->SetIsEngineAsset(false);
			return mesh;
		}

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Mesh>& mesh)
		{
			return SerializeSource(filepath, mesh);
		}

		// Parses the (slow) YAML mesh source, then writes a cooked binary sibling so subsequent
		// loads take the fast binary path. Parsing per-vertex YAML for a large (e.g. skinned) mesh
		// can take seconds to tens of seconds; a cooked read is effectively a memcpy.
		static SharedPtr<Mesh> DeserializeSourceAndCook(UUID uuid, const std::filesystem::path& filepath)
		{
			EB_CORE_WARN("Mesh '{}' has no up-to-date cooked binary; loading from YAML source (slow). "
				"Writing a cooked sibling so future loads are fast.", filepath.string());

			auto mesh = DeserializeSource(uuid, filepath);
			if (!mesh)
				return nullptr;

			// Cook-on-load: persist a binary sibling next to the source. Failure is non-fatal —
			// we still return the mesh, we just don't get the speedup on the next load.
			if (!SerializeCooked(filepath, mesh))
				EB_CORE_WARN("Failed to write cooked mesh sibling for '{}'.", filepath.string());

			return mesh;
		}

		static SharedPtr<Mesh> Deserialize(UUID uuid, const std::filesystem::path& filepath)
		{
			switch (AssetSerializationMode::GetRuntimeLoadTier())
			{
			case RuntimeAssetLoadTier::ForceSourceYaml:
				if (filepath.extension() == ".bin" || MeshFileLooksBinary(filepath))
					return DeserializeCooked(uuid, filepath);
				return DeserializeSource(uuid, filepath);
			case RuntimeAssetLoadTier::ForceCookedBinary:
			{
				auto cookedPath = filepath;
				cookedPath.replace_extension(".bin");
				if (auto cooked = DeserializeCooked(uuid, cookedPath))
					return cooked;
				// Cook missing/corrupt — fall back to the YAML source instead of failing.
				return DeserializeSource(uuid, filepath);
			}
			case RuntimeAssetLoadTier::Auto:
			default:
				// The registered path already points at cooked binary data.
				if (filepath.extension() == ".bin" || MeshFileLooksBinary(filepath))
					return DeserializeCooked(uuid, filepath);

				// Source path (e.g. .ebmesh): prefer a fresh cooked sibling when one exists — binary
				// loading is orders of magnitude faster than parsing per-vertex YAML.
				if (auto cookedPath = GetCookedMeshPath(filepath); IsCookedMeshFresh(filepath, cookedPath))
					return DeserializeCooked(uuid, cookedPath);

				// No usable cook: parse the YAML source once, then cook it for next time.
				return DeserializeSourceAndCook(uuid, filepath);
			}
		}
	};
}